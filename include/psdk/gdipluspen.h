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
    return PenAlignmentCenter;
  }

  Brush *GetBrush(VOID)
  {
    return NULL;
  }

  Status GetColor(Color *color)
  {
    return NotImplemented;
  }

  Status GetCompoundArray(REAL *compoundArray, INT count)
  {
    return NotImplemented;
  }

  INT GetCompoundArrayCount(VOID)
  {
    return 0;
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
    return DashCapFlat;
  }

  REAL GetDashOffset(VOID)
  {
    return 0;
  }

  Status GetDashPattern(REAL *dashArray, INT count)
  {
    return NotImplemented;
  }

  INT GetDashPatternCount(VOID)
  {
    return 0;
  }

  DashStyle GetDashStyle(VOID)
  {
    return DashStyleSolid;
  }

  LineCap GetEndCap(VOID)
  {
    return LineCapFlat;
  }

  Status GetLastStatus(VOID)
  {
    return NotImplemented;
  }

  LineJoin GetLineJoin(VOID)
  {
    return LineJoinMiter;
  }

  REAL GetMiterLimit(VOID)
  {
    return 0;
  }

  PenType GetPenType(VOID)
  {
    return PenTypeSolidColor;
  }

  LineCap GetStartCap(VOID)
  {
    return LineCapFlat;
  }

  Status GetTransform(Matrix *matrix)
  {
    return NotImplemented;
  }

  REAL GetWidth(VOID)
  {
    return 0;
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

  Status SetAlignment(PenAlignment penAlignment)
  {
    return NotImplemented;
  }

  Status SetBrush(const Brush *brush)
  {
    return NotImplemented;
  }

  Status SetColor(const Color &color)
  {
    return NotImplemented;
  }

  Status SetCompoundArray(const REAL *compoundArray, INT count)
  {
    return NotImplemented;
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
    return NotImplemented;
  }

  Status SetDashOffset(REAL dashOffset)
  {
    return NotImplemented;
  }

  Status SetDashPattern(const REAL *dashArray, INT count)
  {
    return NotImplemented;
  }

  Status SetDashStyle(DashStyle dashStyle)
  {
    return NotImplemented;
  }

  Status SetEndCap(LineCap endCap)
  {
    return NotImplemented;
  }

  Status SetLineCap(LineCap startCap, LineCap endCap, DashCap dashCap)
  {
    return NotImplemented;
  }

  Status SetLineJoin(LineJoin lineJoin)
  {
    return NotImplemented;
  }

  Status SetMiterLimit(REAL miterLimit)
  {
    return NotImplemented;
  }

  Status SetStartCap(LineCap startCap)
  {
    return NotImplemented;
  }

  Status SetTransform(const Matrix *matrix)
  {
    return NotImplemented;
  }

  Status SetWidth(REAL width)
  {
    return NotImplemented;
  }

private:
  Status status;
  GpPen *pen;
};

#endif /* _GDIPLUSPEN_H */
