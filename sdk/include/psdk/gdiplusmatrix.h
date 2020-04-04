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
    friend class Brush;
    friend class LinearGradientBrush;
    friend class TextureBrush;

  public:
    Matrix(const RectF &rect, const PointF *dstplg)
    {
        lastStatus = DllExports::GdipCreateMatrix3(&rect, dstplg, &nativeMatrix);
    }

    Matrix(const Rect &rect, const Point *dstplg)
    {
        lastStatus = DllExports::GdipCreateMatrix3I(&rect, dstplg, &nativeMatrix);
    }

    Matrix()
    {
        lastStatus = DllExports::GdipCreateMatrix(&nativeMatrix);
    }

    Matrix(REAL m11, REAL m12, REAL m21, REAL m22, REAL dx, REAL dy)
    {
        lastStatus = DllExports::GdipCreateMatrix2(m11, m12, m21, m22, dx, dy, &nativeMatrix);
    }

    Matrix *
    Clone() const
    {
        GpMatrix *cloneMatrix = NULL;
        SetStatus(DllExports::GdipCloneMatrix(nativeMatrix, &cloneMatrix));

        if (lastStatus != Ok)
            return NULL;

        Matrix *newMatrix = new Matrix(cloneMatrix);
        if (!newMatrix)
            DllExports::GdipDeleteMatrix(cloneMatrix);

        return newMatrix;
    }

    ~Matrix()
    {
        DllExports::GdipDeleteMatrix(nativeMatrix);
    }

    BOOL
    Equals(const Matrix *matrix) const
    {
        BOOL result;
        SetStatus(DllExports::GdipIsMatrixEqual(nativeMatrix, matrix ? getNat(matrix) : NULL, &result));
        return result;
    }

    Status
    GetElements(REAL *m) const
    {
        return SetStatus(DllExports::GdipGetMatrixElements(nativeMatrix, m));
    }

    Status
    GetLastStatus() const
    {
        return lastStatus;
    }

    Status
    Invert()
    {
        return SetStatus(DllExports::GdipInvertMatrix(nativeMatrix));
    }

    BOOL
    IsIdentity() const
    {
        BOOL result;
        SetStatus(DllExports::GdipIsMatrixIdentity(nativeMatrix, &result));
        return result;
    }

    BOOL
    IsInvertible() const
    {
        BOOL result;
        SetStatus(DllExports::GdipIsMatrixInvertible(nativeMatrix, &result));
        return result;
    }

    Status
    Multiply(const Matrix *matrix, MatrixOrder order = MatrixOrderPrepend)
    {
        return SetStatus(DllExports::GdipMultiplyMatrix(nativeMatrix, matrix ? getNat(matrix) : NULL, order));
    }

    REAL
    OffsetX() const
    {
        REAL elements[6];
        if (GetElements(elements) == Ok)
            return elements[4];
        return 0.0f;
    }

    REAL
    OffsetY() const
    {
        REAL elements[6];
        if (GetElements(elements) == Ok)
            return elements[5];
        return 0.0f;
    }

    Status
    Reset()
    {
        return SetStatus(DllExports::GdipSetMatrixElements(nativeMatrix, 1.0, 0.0, 0.0, 1.0, 0.0, 0.0));
    }

    Status
    Rotate(REAL angle, MatrixOrder order = MatrixOrderPrepend)
    {
        return SetStatus(DllExports::GdipRotateMatrix(nativeMatrix, angle, order));
    }

    Status
    RotateAt(REAL angle, const PointF &center, MatrixOrder order = MatrixOrderPrepend)
    {
        if (order == MatrixOrderPrepend)
        {
            SetStatus(DllExports::GdipTranslateMatrix(nativeMatrix, center.X, center.Y, order));
            SetStatus(DllExports::GdipRotateMatrix(nativeMatrix, angle, order));
            return SetStatus(DllExports::GdipTranslateMatrix(nativeMatrix, -center.X, -center.Y, order));
        }
        else
        {
            SetStatus(DllExports::GdipTranslateMatrix(nativeMatrix, -center.X, -center.Y, order));
            SetStatus(DllExports::GdipRotateMatrix(nativeMatrix, angle, order));
            return SetStatus(DllExports::GdipTranslateMatrix(nativeMatrix, center.X, center.Y, order));
        }
    }

    Status
    Scale(REAL scaleX, REAL scaleY, MatrixOrder order = MatrixOrderPrepend)
    {
        return SetStatus(DllExports::GdipScaleMatrix(nativeMatrix, scaleX, scaleY, order));
    }

    Status
    SetElements(REAL m11, REAL m12, REAL m21, REAL m22, REAL dx, REAL dy)
    {
        return SetStatus(DllExports::GdipSetMatrixElements(nativeMatrix, m11, m12, m21, m22, dx, dy));
    }

    Status
    Shear(REAL shearX, REAL shearY, MatrixOrder order = MatrixOrderPrepend)
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
    Translate(REAL offsetX, REAL offsetY, MatrixOrder order = MatrixOrderPrepend)
    {
        return SetStatus(DllExports::GdipTranslateMatrix(nativeMatrix, offsetX, offsetY, order));
    }

  protected:
    GpMatrix *nativeMatrix;
    mutable Status lastStatus;

    Matrix(GpMatrix *matrix) : nativeMatrix(matrix), lastStatus(Ok)
    {
    }

    Status
    SetStatus(Status status) const
    {
        if (status != Ok)
            lastStatus = status;
        return status;
    }

    // get native
    friend inline GpMatrix *&
    getNat(const Matrix *matrix)
    {
        return const_cast<Matrix *>(matrix)->nativeMatrix;
    }
};

#endif /* _GDIPLUSMATRIX_H */
