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
    friend class Pen;
    friend class Region;
    friend class GraphicsPath;

  public:
    Matrix(const RectF &rect, const PointF *dstplg)
    {
        status = DllExports::GdipCreateMatrix3(&rect, dstplg, &nativeMatrix);
    }

    Matrix(const Rect &rect, const Point *dstplg)
    {
        status = DllExports::GdipCreateMatrix3I(&rect, dstplg, &nativeMatrix);
    }

    Matrix(VOID)
    {
        status = DllExports::GdipCreateMatrix(&nativeMatrix);
    }

    Matrix(REAL m11, REAL m12, REAL m21, REAL m22, REAL dx, REAL dy)
    {
        status = DllExports::GdipCreateMatrix2(m11, m12, m21, m22, dx, dy, &nativeMatrix);
    }

    Matrix *Clone(VOID)
    {
        Matrix *cloneMatrix = new Matrix(); // FIXME: Matrix::nativeMatrix already initialized --> potential memory leak
        cloneMatrix->status =
            DllExports::GdipCloneMatrix(nativeMatrix, cloneMatrix ? &cloneMatrix->nativeMatrix : NULL);
        return cloneMatrix;
    }

    ~Matrix(VOID)
    {
        DllExports::GdipDeleteMatrix(nativeMatrix);
    }

    BOOL
    Equals(const Matrix *nativeMatrix)
    {
        BOOL result;
        SetStatus(DllExports::GdipIsMatrixEqual(
            this->nativeMatrix, nativeMatrix ? nativeMatrix->nativeMatrix : NULL, &result));
        return result;
    }

    Status
    GetElements(REAL *m) const
    {
        return SetStatus(DllExports::GdipGetMatrixElements(nativeMatrix, m));
    }

    Status GetLastStatus(VOID)
    {
        return status;
    }

    Status Invert(VOID)
    {
        return SetStatus(DllExports::GdipInvertMatrix(nativeMatrix));
    }

    BOOL IsIdentity(VOID)
    {
        BOOL result;
        SetStatus(DllExports::GdipIsMatrixIdentity(nativeMatrix, &result));
        return result;
    }

    BOOL IsInvertible(VOID)
    {
        BOOL result;
        SetStatus(DllExports::GdipIsMatrixInvertible(nativeMatrix, &result));
        return result;
    }

    Status
    Multiply(const Matrix *nativeMatrix, MatrixOrder order)
    {
        return SetStatus(DllExports::GdipMultiplyMatrix(
            this->nativeMatrix, nativeMatrix ? nativeMatrix->nativeMatrix : NULL, order));
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

    Status
    Rotate(REAL angle, MatrixOrder order)
    {
        return SetStatus(DllExports::GdipRotateMatrix(nativeMatrix, angle, order));
    }

    Status
    RotateAt(REAL angle, const PointF &center, MatrixOrder order)
    {
        return NotImplemented;
    }

    Status
    Scale(REAL scaleX, REAL scaleY, MatrixOrder order)
    {
        return SetStatus(DllExports::GdipScaleMatrix(nativeMatrix, scaleX, scaleY, order));
    }

    Status
    SetElements(REAL m11, REAL m12, REAL m21, REAL m22, REAL dx, REAL dy)
    {
        return SetStatus(DllExports::GdipSetMatrixElements(nativeMatrix, m11, m12, m21, m22, dx, dy));
    }

    Status
    Shear(REAL shearX, REAL shearY, MatrixOrder order)
    {
        return SetStatus(DllExports::GdipShearMatrix(nativeMatrix, shearX, shearY, order));
    }

    Status
    TransformPoints(Point *pts, INT count)
    {
        return SetStatus(DllExports::GdipTransformMatrixPointsI(nativeMatrix, pts, count));
    }

    Status
    TransformPoints(PointF *pts, INT count)
    {
        return SetStatus(DllExports::GdipTransformMatrixPoints(nativeMatrix, pts, count));
    }

    Status
    TransformVectors(Point *pts, INT count)
    {
        return SetStatus(DllExports::GdipVectorTransformMatrixPointsI(nativeMatrix, pts, count));
    }

    Status
    TransformVectors(PointF *pts, INT count)
    {
        return SetStatus(DllExports::GdipVectorTransformMatrixPoints(nativeMatrix, pts, count));
    }

    Status
    Translate(REAL offsetX, REAL offsetY, MatrixOrder order)
    {
        return SetStatus(DllExports::GdipTranslateMatrix(nativeMatrix, offsetX, offsetY, order));
    }

  private:
    mutable Status status;
    GpMatrix *nativeMatrix;

    Status
    SetStatus(Status status) const
    {
        if (status == Ok)
            return status;
        this->status = status;
        return status;
    }
};

#endif /* _GDIPLUSMATRIX_H */
