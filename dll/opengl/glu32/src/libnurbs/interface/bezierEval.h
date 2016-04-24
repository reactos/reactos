/*
 * SGI FREE SOFTWARE LICENSE B (Version 2.0, Sept. 18, 2008)
 * Copyright (C) 1991-2000 Silicon Graphics, Inc. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice including the dates of first publication and
 * either this permission notice or a reference to
 * http://oss.sgi.com/projects/FreeB/
 * shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * SILICON GRAPHICS, INC. BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Except as contained in this notice, the name of Silicon Graphics, Inc.
 * shall not be used in advertising or otherwise to promote the sale, use or
 * other dealings in this Software without prior written authorization from
 * Silicon Graphics, Inc.
 */
/*
*/

#ifndef _BEZIEREVAL_H
#define _BEZIEREVAL_H

void bezierCurveEval(float u0, float u1, int order, float *ctlpoints, int stride,  int dimension, float u, float retpoint[]);
void bezierCurveEvalDer(float u0, float u1, int order, float *ctlpoints, int stride,  int dimension, float u, float retDer[]);
void bezierCurveEvalDerGen(int der, float u0, float u1, int order, float *ctlpoints, int stride,  int dimension, float u, float retDer[]);


void bezierSurfEvalDerGen(int uder, int vder, float u0, float u1, int uorder, float v0, float v1, int vorder, int dimension, float *ctlpoints, int ustride, int vstride, float u, float v, float ret[]);

void bezierSurfEval(float u0, float u1, int uorder, float v0, float v1, int vorder, int dimension, float *ctlpoints, int ustride, int vstride, float u, float v, float ret[]);

void bezierSurfEvalNormal(float u0, float u1, int uorder, float v0, float v1, int vorder, int dimension, float *ctlpoints, int ustride, int vstride, float u, float v, float retNormal[]);


#endif
