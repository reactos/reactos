/*
 * GdiPlusMatrix.h
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

#ifndef _GDIPLUSMATRIX_H
#define _GDIPLUSMATRIX_H

class Matrix : public GdiplusBase
{
public:
  Matrix(const RectF &rect, const PointF *dstplg)
  {
  }

  Matrix(const Rect &rect, const Point *dstplg)
  {
  }

  Matrix(VOID)
  {
  }

  Matrix(REAL m11, REAL m12, REAL m21, REAL m22, REAL dx, REAL dy)
  {
  }

  Matrix *Clone(VOID)
  {
    return NULL;
  }

  static BOOL Equals(const Matrix* matrix)
  {
    return FALSE;
  }

  Status GetElements(REAL *m) const
  {
    return NotImplemented;
  }

  Status GetLastStatus(VOID)
  {
    return NotImplemented;
  }

  Status Invert(VOID)
  {
    return NotImplemented;
  }

  BOOL IsIdentity(VOID)
  {
    return FALSE;
  }

  BOOL IsInvertible(VOID)
  {
    return FALSE;
  }

  Status Multiply(const Matrix *matrix, MatrixOrder order)
  {
    return NotImplemented;
  }

  REAL OffsetX(VOID)
  {
    return 0;
  }

  REAL OffsetY(VOID)
  {
    return 0;
  }

  Status Reset(VOID)
  {
    return NotImplemented;
  }

  Status Rotate(REAL angle, MatrixOrder order)
  {
    return NotImplemented;
  }

  Status RotateAt(REAL angle, const PointF &center, MatrixOrder order)
  {
    return NotImplemented;
  }

  Status Scale(REAL scaleX, REAL scaleY, MatrixOrder order)
  {
    return NotImplemented;
  }

  Status SetElements(REAL m11, REAL m12, REAL m21, REAL m22, REAL dx, REAL dy)
  {
    return NotImplemented;
  }

  Status Shear(REAL shearX, REAL shearY, REAL order)
  {
    return NotImplemented;
  }

  Status TransformPoints(Point *pts, INT count)
  {
    return NotImplemented;
  }

  Status TransformPoints(PointF *pts, INT count)
  {
    return NotImplemented;
  }

  Status TransformVectors(Point *pts, INT count)
  {
    return NotImplemented;
  }

  Status TransformVectors(PointF *pts, INT count)
  {
    return NotImplemented;
  }

  Status Translate(REAL offsetX, REAL offsetY, REAL order)
  {
    return NotImplemented;
  }
};

#endif /* _GDIPLUSMATRIX_H */
