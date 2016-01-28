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
    status = DllExports::GdipCreateMatrix3(&rect, dstplg, &matrix);
  }

  Matrix(const Rect &rect, const Point *dstplg)
  {
    status = DllExports::GdipCreateMatrix3I(&rect, dstplg, &matrix);
  }

  Matrix(VOID)
  {
    status = DllExports::GdipCreateMatrix(&matrix);
  }

  Matrix(REAL m11, REAL m12, REAL m21, REAL m22, REAL dx, REAL dy)
  {
    status = DllExports::GdipCreateMatrix2(m11, m12, m21, m22, dx, dy, &matrix);
  }

  Matrix *Clone(VOID)
  {
    Matrix *cloneMatrix = new Matrix();  // FIXME: Matrix::matrix already initialized --> potential memory leak
    cloneMatrix->status = DllExports::GdipCloneMatrix(matrix, &cloneMatrix);
    return cloneMatrix;
  }

  ~Matrix(VOID)
  {
    DllExports::GdipDeleteMatrix(matrix);
  }

  BOOL Equals(const Matrix* matrix)
  {
    BOOL result;
    SetStatus(DllExports::GdipIsMatrixEqual(this->matrix, matrix->matrix, &result));
    return result;
  }

  Status GetElements(REAL *m) const
  {
    return SetStatus(DllExports::GdipGetMatrixElements(matrix, m));
  }

  Status GetLastStatus(VOID)
  {
    return status;
  }

  Status Invert(VOID)
  {
    return SetStatus(DllExports::GdipInvertMatrix(matrix));
  }

  BOOL IsIdentity(VOID)
  {
    BOOL result;
    SetStatus(DllExports::GdipIsMatrixIdentity(matrix, &result));
    return result;
  }

  BOOL IsInvertible(VOID)
  {
    BOOL result;
    SetStatus(DllExports::GdipIsMatrixInvertible(matrix, &result));
    return result;
  }

  Status Multiply(const Matrix *matrix, MatrixOrder order)
  {
    return SetStatus(DllExports::GdipMultiplyMatrix(this->matrix, matrix->matrix, order));
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
    return SetStatus(DllExports::GdipRotateMatrix(matrix, angle, order));
  }

  Status RotateAt(REAL angle, const PointF &center, MatrixOrder order)
  {
    return NotImplemented;
  }

  Status Scale(REAL scaleX, REAL scaleY, MatrixOrder order)
  {
    return SetStatus(DllExports::GdipScaleMatrix(matrix, scaleX, scaleY, order));
  }

  Status SetElements(REAL m11, REAL m12, REAL m21, REAL m22, REAL dx, REAL dy)
  {
    return SetStatus(DllExports::GdipSetMatrixElements(matrix, m11, m12, m21, m22, dx, dy));
  }

  Status Shear(REAL shearX, REAL shearY, REAL order)
  {
    return SetStatus(DllExports::GdipShearMatrix(matrix, shearX, shearY, order));
  }

  Status TransformPoints(Point *pts, INT count)
  {
    return SetStatus(DllExports::GdipTransformMatrixPointsI(matrix, pts, count));
  }

  Status TransformPoints(PointF *pts, INT count)
  {
    return SetStatus(DllExports::GdipTransformMatrixPoints(matrix, pts, count));
  }

  Status TransformVectors(Point *pts, INT count)
  {
    return SetStatus(DllExports::GdipVectorTransformMatrixPointsI(matrix, pts, count));
  }

  Status TransformVectors(PointF *pts, INT count)
  {
    return SetStatus(DllExports::GdipVectorTransformMatrixPoints(matrix, pts, count));
  }

  Status Translate(REAL offsetX, REAL offsetY, REAL order)
  {
    return SetStatus(DllExports::GdipTranslateMatrix(matrix, offsetX, offsetY, order));
  }

private:
  mutable Status status;
  GpMatrix *matrix;

  Status SetStatus(Status status) const
  {
    if (status == Ok)
      return status;
    this->status = status;
    return status;
  }
};

#endif /* _GDIPLUSMATRIX_H */
