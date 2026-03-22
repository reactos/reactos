/*
 * Copyright (C) 2007 Google (Evan Stade)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>
#include <math.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"

#include "objbase.h"

#include "gdiplus.h"
#include "gdiplus_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(gdiplus);

/* Multiplies two matrices of the form
 *
 * idx:0 idx:1     0
 * idx:2 idx:3     0
 * idx:4 idx:5     1
 *
 * and puts the output in out.
 * */
static inline void matrix_multiply(GDIPCONST REAL * left, GDIPCONST REAL * right, REAL * out)
{
    REAL temp[6];
    temp[0] = left[0] * right[0] + left[1] * right[2];
    temp[1] = left[0] * right[1] + left[1] * right[3];
    temp[2] = left[2] * right[0] + left[3] * right[2];
    temp[3] = left[2] * right[1] + left[3] * right[3];
    temp[4] = left[4] * right[0] + left[5] * right[2] + right[4];
    temp[5] = left[4] * right[1] + left[5] * right[3] + right[5];
    memcpy(out, temp, 6 * sizeof(REAL));
}

static REAL matrix_det(GDIPCONST GpMatrix *matrix)
{
    return matrix->matrix[0]*matrix->matrix[3] - matrix->matrix[1]*matrix->matrix[2];
}

GpStatus WINGDIPAPI GdipCreateMatrix2(REAL m11, REAL m12, REAL m21, REAL m22,
    REAL dx, REAL dy, GpMatrix **matrix)
{
    TRACE("(%.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %p)\n", m11, m12, m21, m22, dx, dy, matrix);

    if(!matrix)
        return InvalidParameter;

    *matrix = malloc(sizeof(GpMatrix));
    if(!*matrix) return OutOfMemory;

    /* first row */
    (*matrix)->matrix[0] = m11;
    (*matrix)->matrix[1] = m12;
    /* second row */
    (*matrix)->matrix[2] = m21;
    (*matrix)->matrix[3] = m22;
    /* third row */
    (*matrix)->matrix[4] = dx;
    (*matrix)->matrix[5] = dy;

    return Ok;
}

GpStatus WINGDIPAPI GdipCreateMatrix3(GDIPCONST GpRectF *rect,
    GDIPCONST GpPointF *pt, GpMatrix **matrix)
{
    REAL m11, m12, m21, m22, dx, dy;

    TRACE("(%s, %p, %p)\n", debugstr_rectf(rect), pt, matrix);

    if(!matrix || !pt)
        return InvalidParameter;

    m11 = (pt[1].X - pt[0].X) / rect->Width;
    m21 = (pt[2].X - pt[0].X) / rect->Height;
    dx = pt[0].X - m11 * rect->X - m21 * rect->Y;
    m12 = (pt[1].Y - pt[0].Y) / rect->Width;
    m22 = (pt[2].Y - pt[0].Y) / rect->Height;
    dy = pt[0].Y - m12 * rect->X - m22 * rect->Y;

    return GdipCreateMatrix2(m11, m12, m21, m22, dx, dy, matrix);
}

GpStatus WINGDIPAPI GdipCreateMatrix3I(GDIPCONST GpRect *rect, GDIPCONST GpPoint *pt,
    GpMatrix **matrix)
{
    GpRectF rectF;
    GpPointF ptF[3];
    int i;

    TRACE("(%p, %p, %p)\n", rect, pt, matrix);

    set_rect(&rectF, rect->X, rect->Y, rect->Width, rect->Height);

    for (i = 0; i < 3; i++) {
        ptF[i].X = (REAL)pt[i].X;
        ptF[i].Y = (REAL)pt[i].Y;
    }
    return GdipCreateMatrix3(&rectF, ptF, matrix);
}

GpStatus WINGDIPAPI GdipCloneMatrix(GpMatrix *matrix, GpMatrix **clone)
{
    TRACE("(%s, %p)\n", debugstr_matrix(matrix), clone);

    if(!matrix || !clone)
        return InvalidParameter;

    *clone = malloc(sizeof(GpMatrix));
    if(!*clone) return OutOfMemory;

    **clone = *matrix;

    return Ok;
}

GpStatus WINGDIPAPI GdipCreateMatrix(GpMatrix **matrix)
{
    TRACE("(%p)\n", matrix);

    if(!matrix)
        return InvalidParameter;

    *matrix = malloc(sizeof(GpMatrix));
    if(!*matrix) return OutOfMemory;

    (*matrix)->matrix[0] = 1.0;
    (*matrix)->matrix[1] = 0.0;
    (*matrix)->matrix[2] = 0.0;
    (*matrix)->matrix[3] = 1.0;
    (*matrix)->matrix[4] = 0.0;
    (*matrix)->matrix[5] = 0.0;

    return Ok;
}

GpStatus WINGDIPAPI GdipDeleteMatrix(GpMatrix *matrix)
{
    TRACE("(%p)\n", matrix);

    if(!matrix)
        return InvalidParameter;

    free(matrix);

    return Ok;
}

GpStatus WINGDIPAPI GdipGetMatrixElements(GDIPCONST GpMatrix *matrix,
    REAL *out)
{
    TRACE("(%s, %p)\n", debugstr_matrix(matrix), out);

    if(!matrix || !out)
        return InvalidParameter;

    memcpy(out, matrix->matrix, sizeof(matrix->matrix));

    return Ok;
}

GpStatus WINGDIPAPI GdipInvertMatrix(GpMatrix *matrix)
{
    GpMatrix copy;
    REAL det;

    TRACE("(%s)\n", debugstr_matrix(matrix));

    if(!matrix)
        return InvalidParameter;

    /* optimize inverting simple scaling and translation matrices */
    if(matrix->matrix[1] == 0 && matrix->matrix[2] == 0)
    {
        if (matrix->matrix[0] != 0 && matrix->matrix[3] != 0)
        {
            matrix->matrix[4] = -matrix->matrix[4] / matrix->matrix[0];
            matrix->matrix[5] = -matrix->matrix[5] / matrix->matrix[3];
            matrix->matrix[0] = 1 / matrix->matrix[0];
            matrix->matrix[3] = 1 / matrix->matrix[3];

            return Ok;
        }
        else
            return InvalidParameter;
    }
    det = matrix_det(matrix);
    if (!(fabs(det) >= 1e-5))
        return InvalidParameter;

    det = 1 / det;

    copy = *matrix;
    /* store result */
    matrix->matrix[0] =   copy.matrix[3] * det;
    matrix->matrix[1] =  -copy.matrix[1] * det;
    matrix->matrix[2] =  -copy.matrix[2] * det;
    matrix->matrix[3] =   copy.matrix[0] * det;
    matrix->matrix[4] =  (copy.matrix[2]*copy.matrix[5]-copy.matrix[3]*copy.matrix[4]) * det;
    matrix->matrix[5] = -(copy.matrix[0]*copy.matrix[5]-copy.matrix[1]*copy.matrix[4]) * det;

    return Ok;
}

GpStatus WINGDIPAPI GdipIsMatrixInvertible(GDIPCONST GpMatrix *matrix, BOOL *result)
{
    TRACE("(%s, %p)\n", debugstr_matrix(matrix), result);

    if(!matrix || !result)
        return InvalidParameter;

    if(matrix->matrix[1] == 0 && matrix->matrix[2] == 0)
        *result = matrix->matrix[0] != 0 && matrix->matrix[3] != 0;
    else
        *result = (fabs(matrix_det(matrix)) >= 1e-5);

    return Ok;
}

GpStatus WINGDIPAPI GdipMultiplyMatrix(GpMatrix *matrix, GDIPCONST GpMatrix* matrix2,
    GpMatrixOrder order)
{
    TRACE("(%s, %s, %d)\n", debugstr_matrix(matrix), debugstr_matrix(matrix2), order);

    if(!matrix || !matrix2)
        return InvalidParameter;

    if(order == MatrixOrderAppend)
        matrix_multiply(matrix->matrix, matrix2->matrix, matrix->matrix);
    else if (order == MatrixOrderPrepend)
        matrix_multiply(matrix2->matrix, matrix->matrix, matrix->matrix);
    else
        return InvalidParameter;

    return Ok;
}

GpStatus WINGDIPAPI GdipRotateMatrix(GpMatrix *matrix, REAL angle,
    GpMatrixOrder order)
{
    REAL cos_theta, sin_theta, rotate[6];

    TRACE("(%p, %.2f, %d)\n", debugstr_matrix(matrix), angle, order);

    if(!matrix)
        return InvalidParameter;

    angle = deg2rad(angle);
    cos_theta = cos(angle);
    sin_theta = sin(angle);

    rotate[0] = cos_theta;
    rotate[1] = sin_theta;
    rotate[2] = -sin_theta;
    rotate[3] = cos_theta;
    rotate[4] = 0.0;
    rotate[5] = 0.0;

    if(order == MatrixOrderAppend)
        matrix_multiply(matrix->matrix, rotate, matrix->matrix);
    else if (order == MatrixOrderPrepend)
        matrix_multiply(rotate, matrix->matrix, matrix->matrix);
    else
        return InvalidParameter;

    return Ok;
}

GpStatus WINGDIPAPI GdipScaleMatrix(GpMatrix *matrix, REAL scaleX, REAL scaleY,
    GpMatrixOrder order)
{
    TRACE("(%s, %.2f, %.2f, %d)\n", debugstr_matrix(matrix), scaleX, scaleY, order);

    if(!matrix)
        return InvalidParameter;

    if(order == MatrixOrderAppend)
    {
        matrix->matrix[0] *= scaleX;
        matrix->matrix[1] *= scaleY;
        matrix->matrix[2] *= scaleX;
        matrix->matrix[3] *= scaleY;
        matrix->matrix[4] *= scaleX;
        matrix->matrix[5] *= scaleY;
    }
    else if (order == MatrixOrderPrepend)
    {
        matrix->matrix[0] *= scaleX;
        matrix->matrix[1] *= scaleX;
        matrix->matrix[2] *= scaleY;
        matrix->matrix[3] *= scaleY;
    }
    else
        return InvalidParameter;

    return Ok;
}

GpStatus WINGDIPAPI GdipSetMatrixElements(GpMatrix *matrix, REAL m11, REAL m12,
    REAL m21, REAL m22, REAL dx, REAL dy)
{
    TRACE("(%s, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f)\n", debugstr_matrix(matrix), m11, m12,
            m21, m22, dx, dy);

    if(!matrix)
        return InvalidParameter;

    matrix->matrix[0] = m11;
    matrix->matrix[1] = m12;
    matrix->matrix[2] = m21;
    matrix->matrix[3] = m22;
    matrix->matrix[4] = dx;
    matrix->matrix[5] = dy;

    return Ok;
}

GpStatus WINGDIPAPI GdipShearMatrix(GpMatrix *matrix, REAL shearX, REAL shearY,
    GpMatrixOrder order)
{
    REAL shear[6];

    TRACE("(%s, %.2f, %.2f, %d)\n", debugstr_matrix(matrix), shearX, shearY, order);

    if(!matrix)
        return InvalidParameter;

    /* prepare transformation matrix */
    shear[0] = 1.0;
    shear[1] = shearY;
    shear[2] = shearX;
    shear[3] = 1.0;
    shear[4] = 0.0;
    shear[5] = 0.0;

    if(order == MatrixOrderAppend)
        matrix_multiply(matrix->matrix, shear, matrix->matrix);
    else if (order == MatrixOrderPrepend)
        matrix_multiply(shear, matrix->matrix, matrix->matrix);
    else
        return InvalidParameter;

    return Ok;
}

GpStatus WINGDIPAPI GdipTransformMatrixPoints(GpMatrix *matrix, GpPointF *pts,
                                              INT count)
{
    REAL x, y;
    INT i;

    TRACE("(%s, %p, %d)\n", debugstr_matrix(matrix), pts, count);

    if(!matrix || !pts || count <= 0)
        return InvalidParameter;

    for(i = 0; i < count; i++)
    {
        x = pts[i].X;
        y = pts[i].Y;

        pts[i].X = x * matrix->matrix[0] + y * matrix->matrix[2] + matrix->matrix[4];
        pts[i].Y = x * matrix->matrix[1] + y * matrix->matrix[3] + matrix->matrix[5];
    }

    return Ok;
}

GpStatus WINGDIPAPI GdipTransformMatrixPointsI(GpMatrix *matrix, GpPoint *pts, INT count)
{
    GpPointF *ptsF;
    GpStatus ret;
    INT i;

    TRACE("(%s, %p, %d)\n", debugstr_matrix(matrix), pts, count);

    if(count <= 0)
        return InvalidParameter;

    ptsF = malloc(sizeof(GpPointF) * count);
    if(!ptsF)
        return OutOfMemory;

    for(i = 0; i < count; i++){
        ptsF[i].X = (REAL)pts[i].X;
        ptsF[i].Y = (REAL)pts[i].Y;
    }

    ret = GdipTransformMatrixPoints(matrix, ptsF, count);

    if(ret == Ok)
        for(i = 0; i < count; i++){
            pts[i].X = gdip_round(ptsF[i].X);
            pts[i].Y = gdip_round(ptsF[i].Y);
        }
    free(ptsF);

    return ret;
}

GpStatus WINGDIPAPI GdipTranslateMatrix(GpMatrix *matrix, REAL offsetX,
    REAL offsetY, GpMatrixOrder order)
{
    TRACE("(%s, %.2f, %.2f, %d)\n", debugstr_matrix(matrix), offsetX, offsetY, order);

    if(!matrix)
        return InvalidParameter;

    if(order == MatrixOrderAppend)
    {
        matrix->matrix[4] += offsetX;
        matrix->matrix[5] += offsetY;
    }
    else if (order == MatrixOrderPrepend)
    {
        matrix->matrix[4] = offsetX * matrix->matrix[0] + offsetY * matrix->matrix[2]
            + matrix->matrix[4];
        matrix->matrix[5] = offsetX * matrix->matrix[1] + offsetY * matrix->matrix[3]
            + matrix->matrix[5];
    }
    else
        return InvalidParameter;

    return Ok;
}

GpStatus WINGDIPAPI GdipVectorTransformMatrixPoints(GpMatrix *matrix, GpPointF *pts, INT count)
{
    REAL x, y;
    INT i;

    TRACE("(%s, %p, %d)\n", debugstr_matrix(matrix), pts, count);

    if(!matrix || !pts || count <= 0)
        return InvalidParameter;

    for(i = 0; i < count; i++)
    {
        x = pts[i].X;
        y = pts[i].Y;

        pts[i].X = x * matrix->matrix[0] + y * matrix->matrix[2];
        pts[i].Y = x * matrix->matrix[1] + y * matrix->matrix[3];
    }

    return Ok;
}

GpStatus WINGDIPAPI GdipVectorTransformMatrixPointsI(GpMatrix *matrix, GpPoint *pts, INT count)
{
    GpPointF *ptsF;
    GpStatus ret;
    INT i;

    TRACE("(%s, %p, %d)\n", debugstr_matrix(matrix), pts, count);

    if(count <= 0)
        return InvalidParameter;

    ptsF = malloc(sizeof(GpPointF) * count);
    if(!ptsF)
        return OutOfMemory;

    for(i = 0; i < count; i++){
        ptsF[i].X = (REAL)pts[i].X;
        ptsF[i].Y = (REAL)pts[i].Y;
    }

    ret = GdipVectorTransformMatrixPoints(matrix, ptsF, count);
    /* store back */
    if(ret == Ok)
        for(i = 0; i < count; i++){
            pts[i].X = gdip_round(ptsF[i].X);
            pts[i].Y = gdip_round(ptsF[i].Y);
        }
    free(ptsF);

    return ret;
}

GpStatus WINGDIPAPI GdipIsMatrixEqual(GDIPCONST GpMatrix *matrix, GDIPCONST GpMatrix *matrix2,
    BOOL *result)
{
    TRACE("(%s, %s, %p)\n", debugstr_matrix(matrix), debugstr_matrix(matrix2), result);

    if(!matrix || !matrix2 || !result)
        return InvalidParameter;
    /* based on single array member of GpMatrix */
    *result = (memcmp(matrix->matrix, matrix2->matrix, sizeof(GpMatrix)) == 0);

    return Ok;
}

GpStatus WINGDIPAPI GdipIsMatrixIdentity(GDIPCONST GpMatrix *matrix, BOOL *result)
{
    static const GpMatrix identity =
    {
      { 1.0, 0.0,
        0.0, 1.0,
        0.0, 0.0 }
    };

    TRACE("(%s, %p)\n", debugstr_matrix(matrix), result);

    if(!matrix || !result)
        return InvalidParameter;

    return GdipIsMatrixEqual(matrix, &identity, result);
}
