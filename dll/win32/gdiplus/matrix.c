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
static void matrix_multiply(GDIPCONST REAL * left, GDIPCONST REAL * right, REAL * out)
{
    REAL temp[6];
    int i, odd;

    for(i = 0; i < 6; i++){
        odd = i % 2;
        temp[i] = left[i - odd] * right[odd] + left[i - odd + 1] * right[odd + 2] +
                  (i >= 4 ? right[odd + 4] : 0.0);
    }

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

    *matrix = GdipAlloc(sizeof(GpMatrix));
    if(!*matrix)    return OutOfMemory;

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
    TRACE("(%p, %p, %p)\n", rect, pt, matrix);

    if(!matrix)
        return InvalidParameter;

    *matrix = GdipAlloc(sizeof(GpMatrix));
    if(!*matrix)    return OutOfMemory;

    memcpy((*matrix)->matrix, rect, 4 * sizeof(REAL));

    (*matrix)->matrix[4] = pt->X;
    (*matrix)->matrix[5] = pt->Y;

    return Ok;
}

GpStatus WINGDIPAPI GdipCreateMatrix3I(GDIPCONST GpRect *rect, GDIPCONST GpPoint *pt,
    GpMatrix **matrix)
{
    GpRectF rectF;
    GpPointF ptF;

    TRACE("(%p, %p, %p)\n", rect, pt, matrix);

    rectF.X = (REAL)rect->X;
    rectF.Y = (REAL)rect->Y;
    rectF.Width = (REAL)rect->Width;
    rectF.Height = (REAL)rect->Height;

    ptF.X = (REAL)pt->X;
    ptF.Y = (REAL)pt->Y;

    return GdipCreateMatrix3(&rectF, &ptF, matrix);
}

GpStatus WINGDIPAPI GdipCloneMatrix(GpMatrix *matrix, GpMatrix **clone)
{
    TRACE("(%p, %p)\n", matrix, clone);

    if(!matrix || !clone)
        return InvalidParameter;

    *clone = GdipAlloc(sizeof(GpMatrix));
    if(!*clone)    return OutOfMemory;

    **clone = *matrix;

    return Ok;
}

GpStatus WINGDIPAPI GdipCreateMatrix(GpMatrix **matrix)
{
    TRACE("(%p)\n", matrix);

    if(!matrix)
        return InvalidParameter;

    *matrix = GdipAlloc(sizeof(GpMatrix));
    if(!*matrix)    return OutOfMemory;

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

    GdipFree(matrix);

    return Ok;
}

GpStatus WINGDIPAPI GdipGetMatrixElements(GDIPCONST GpMatrix *matrix,
    REAL *out)
{
    TRACE("(%p, %p)\n", matrix, out);

    if(!matrix || !out)
        return InvalidParameter;

    memcpy(out, matrix->matrix, sizeof(matrix->matrix));

    return Ok;
}

GpStatus WINGDIPAPI GdipInvertMatrix(GpMatrix *matrix)
{
    GpMatrix copy;
    REAL det;
    BOOL invertible;

    TRACE("(%p)\n", matrix);

    if(!matrix)
        return InvalidParameter;

    GdipIsMatrixInvertible(matrix, &invertible);
    if(!invertible)
        return InvalidParameter;

    det = matrix_det(matrix);

    copy = *matrix;
    /* store result */
    matrix->matrix[0] =   copy.matrix[3] / det;
    matrix->matrix[1] =  -copy.matrix[1] / det;
    matrix->matrix[2] =  -copy.matrix[2] / det;
    matrix->matrix[3] =   copy.matrix[0] / det;
    matrix->matrix[4] =  (copy.matrix[2]*copy.matrix[5]-copy.matrix[3]*copy.matrix[4]) / det;
    matrix->matrix[5] = -(copy.matrix[0]*copy.matrix[5]-copy.matrix[1]*copy.matrix[4]) / det;

    return Ok;
}

GpStatus WINGDIPAPI GdipIsMatrixInvertible(GDIPCONST GpMatrix *matrix, BOOL *result)
{
    TRACE("(%p, %p)\n", matrix, result);

    if(!matrix || !result)
        return InvalidParameter;

    *result = (fabs(matrix_det(matrix)) >= 1e-5);

    return Ok;
}

GpStatus WINGDIPAPI GdipMultiplyMatrix(GpMatrix *matrix, GpMatrix* matrix2,
    GpMatrixOrder order)
{
    TRACE("(%p, %p, %d)\n", matrix, matrix2, order);

    if(!matrix || !matrix2)
        return InvalidParameter;

    if(order == MatrixOrderAppend)
        matrix_multiply(matrix->matrix, matrix2->matrix, matrix->matrix);
    else
        matrix_multiply(matrix2->matrix, matrix->matrix, matrix->matrix);

    return Ok;
}

GpStatus WINGDIPAPI GdipRotateMatrix(GpMatrix *matrix, REAL angle,
    GpMatrixOrder order)
{
    REAL cos_theta, sin_theta, rotate[6];

    TRACE("(%p, %.2f, %d)\n", matrix, angle, order);

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
    else
        matrix_multiply(rotate, matrix->matrix, matrix->matrix);

    return Ok;
}

GpStatus WINGDIPAPI GdipScaleMatrix(GpMatrix *matrix, REAL scaleX, REAL scaleY,
    GpMatrixOrder order)
{
    REAL scale[6];

    TRACE("(%p, %.2f, %.2f, %d)\n", matrix, scaleX, scaleY, order);

    if(!matrix)
        return InvalidParameter;

    scale[0] = scaleX;
    scale[1] = 0.0;
    scale[2] = 0.0;
    scale[3] = scaleY;
    scale[4] = 0.0;
    scale[5] = 0.0;

    if(order == MatrixOrderAppend)
        matrix_multiply(matrix->matrix, scale, matrix->matrix);
    else
        matrix_multiply(scale, matrix->matrix, matrix->matrix);

    return Ok;
}

GpStatus WINGDIPAPI GdipSetMatrixElements(GpMatrix *matrix, REAL m11, REAL m12,
    REAL m21, REAL m22, REAL dx, REAL dy)
{
    TRACE("(%p, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f)\n", matrix, m11, m12,
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

    TRACE("(%p, %.2f, %.2f, %d)\n", matrix, shearX, shearY, order);

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
    else
        matrix_multiply(shear, matrix->matrix, matrix->matrix);

    return Ok;
}

GpStatus WINGDIPAPI GdipTransformMatrixPoints(GpMatrix *matrix, GpPointF *pts,
                                              INT count)
{
    REAL x, y;
    INT i;

    TRACE("(%p, %p, %d)\n", matrix, pts, count);

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

    TRACE("(%p, %p, %d)\n", matrix, pts, count);

    if(count <= 0)
        return InvalidParameter;

    ptsF = GdipAlloc(sizeof(GpPointF) * count);
    if(!ptsF)
        return OutOfMemory;

    for(i = 0; i < count; i++){
        ptsF[i].X = (REAL)pts[i].X;
        ptsF[i].Y = (REAL)pts[i].Y;
    }

    ret = GdipTransformMatrixPoints(matrix, ptsF, count);

    if(ret == Ok)
        for(i = 0; i < count; i++){
            pts[i].X = roundr(ptsF[i].X);
            pts[i].Y = roundr(ptsF[i].Y);
        }
    GdipFree(ptsF);

    return ret;
}

GpStatus WINGDIPAPI GdipTranslateMatrix(GpMatrix *matrix, REAL offsetX,
    REAL offsetY, GpMatrixOrder order)
{
    REAL translate[6];

    TRACE("(%p, %.2f, %.2f, %d)\n", matrix, offsetX, offsetY, order);

    if(!matrix)
        return InvalidParameter;

    translate[0] = 1.0;
    translate[1] = 0.0;
    translate[2] = 0.0;
    translate[3] = 1.0;
    translate[4] = offsetX;
    translate[5] = offsetY;

    if(order == MatrixOrderAppend)
        matrix_multiply(matrix->matrix, translate, matrix->matrix);
    else
        matrix_multiply(translate, matrix->matrix, matrix->matrix);

    return Ok;
}

GpStatus WINGDIPAPI GdipVectorTransformMatrixPoints(GpMatrix *matrix, GpPointF *pts, INT count)
{
    REAL x, y;
    INT i;

    TRACE("(%p, %p, %d)\n", matrix, pts, count);

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

    TRACE("(%p, %p, %d)\n", matrix, pts, count);

    if(count <= 0)
        return InvalidParameter;

    ptsF = GdipAlloc(sizeof(GpPointF) * count);
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
            pts[i].X = roundr(ptsF[i].X);
            pts[i].Y = roundr(ptsF[i].Y);
        }
    GdipFree(ptsF);

    return ret;
}

GpStatus WINGDIPAPI GdipIsMatrixEqual(GDIPCONST GpMatrix *matrix, GDIPCONST GpMatrix *matrix2,
    BOOL *result)
{
    TRACE("(%p, %p, %p)\n", matrix, matrix2, result);

    if(!matrix || !matrix2 || !result)
        return InvalidParameter;
    /* based on single array member of GpMatrix */
    *result = (memcmp(matrix->matrix, matrix2->matrix, sizeof(GpMatrix)) == 0);

    return Ok;
}

GpStatus WINGDIPAPI GdipIsMatrixIdentity(GDIPCONST GpMatrix *matrix, BOOL *result)
{
    GpMatrix *e;
    GpStatus ret;
    BOOL isIdentity;

    TRACE("(%p, %p)\n", matrix, result);

    if(!matrix || !result)
        return InvalidParameter;

    ret = GdipCreateMatrix(&e);
    if(ret != Ok) return ret;

    ret = GdipIsMatrixEqual(matrix, e, &isIdentity);
    if(ret == Ok)
        *result = isIdentity;

    GdipFree(e);

    return ret;
}
