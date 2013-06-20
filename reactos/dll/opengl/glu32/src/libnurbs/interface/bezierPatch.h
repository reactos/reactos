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

#ifndef _BEZIERPATCH_H
#define _BEZIERPATCH_H

typedef struct bezierPatch{
  float umin, vmin, umax, vmax;
  int uorder; /*order= degree + 1*/
  int vorder; 

  /*
   *the control points are stored in a one dimensional  array.
   *the surface is defined as:
   *      s(u,v) = sum_{i,j} P(i,j) * B_i(u) * B_j(v).
   *where P(i,j) are the control points, B_i(.) are Bezier
   *basis functions.
   *Each control point can have dimension 3 or 4: (x,y,z,w).
   *The components of P(i,j) are stored in a one dimensional 
   *array: 
   *       ctlpoints[]
   *in the order of:
   * P[0,0], P[0,1], ..., P[0,vorder-1],
   * P[1,0], P[1,1], ..., P[1,vorder-1],
   *  ...             
   * P[uorder-1,0], P[uorder-1,1], ..., P[uorder-1,vorder-1].
   */
  int dimension;
  float* ctlpoints;

  /*
   *in case we have to manage multiple bezierPatches.
   */
  struct bezierPatch  *next; 

} bezierPatch;

#ifdef __cplusplus
extern "C" {
#endif

bezierPatch* bezierPatchMake(float umin, float vmin, float umax, float vmax, int urder, int vorder, int dimension);

bezierPatch* bezierPatchMake2(float umin, float vmin, float umax, float vmax, int urder, int vorder, int dimension, int ustride, int vstride, float *ctlpoints);


bezierPatch* bezierPatchInsert(bezierPatch *list, bezierPatch *b);

void bezierPatchDelete(bezierPatch *b);

void bezierPatchDeleteList(bezierPatch *b);

void bezierPatchPrint(bezierPatch *b);

void bezierPatchPrintList(bezierPatch *list);

void bezierPatchEval(bezierPatch *b, float u, float v, float ret[]);

void bezierPatchEvalNormal(bezierPatch *b, float u, float v, float retNormal[]);

void bezierPatchEval(bezierPatch *b, float u, float v, float ret[]);

void bezierPatchEvalNormal(bezierPatch *b, float u, float v, float ret[]);


void bezierPatchDraw(bezierPatch *bpatch, int u_reso, int v_reso);

void bezierPatchListDraw(bezierPatch *list, int u_reso, int v_reso);

#ifdef __cplusplus
}
#endif


#endif
