/*
** License Applicability. Except to the extent portions of this file are
** made subject to an alternative license as permitted in the SGI Free
** Software License B, Version 1.1 (the "License"), the contents of this
** file are subject only to the provisions of the License. You may not use
** this file except in compliance with the License. You may obtain a copy
** of the License at Silicon Graphics, Inc., attn: Legal Services, 1600
** Amphitheatre Parkway, Mountain View, CA 94043-1351, or at:
** 
** http://oss.sgi.com/projects/FreeB
** 
** Note that, as provided in the License, the Software is distributed on an
** "AS IS" basis, with ALL EXPRESS AND IMPLIED WARRANTIES AND CONDITIONS
** DISCLAIMED, INCLUDING, WITHOUT LIMITATION, ANY IMPLIED WARRANTIES AND
** CONDITIONS OF MERCHANTABILITY, SATISFACTORY QUALITY, FITNESS FOR A
** PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
** 
** Original Code. The Original Code is: OpenGL Sample Implementation,
** Version 1.2.1, released January 26, 2000, developed by Silicon Graphics,
** Inc. The Original Code is Copyright (c) 1991-2000 Silicon Graphics, Inc.
** Copyright in any portions created by third parties is as indicated
** elsewhere herein. All Rights Reserved.
** 
** Additional Notice Provisions: The application programming interfaces
** established by SGI in conjunction with the Original Code are The
** OpenGL(R) Graphics System: A Specification (Version 1.2.1), released
** April 1, 1999; The OpenGL(R) Graphics System Utility Library (Version
** 1.3), released November 4, 1998; and OpenGL(R) Graphics with the X
** Window System(R) (Version 1.3), released October 19, 1998. This software
** was created using the OpenGL(R) version 1.2.1 Sample Implementation
** published by SGI, but has not been independently verified as being
** compliant with the OpenGL(R) version 1.2.1 Specification.
**
** $Date: 2004/02/02 16:39:08 $ $Revision: 1.1 $
*/
/*
** $Header: /cygdrive/c/RCVS/CVS/ReactOS/reactos/lib/glu32/libnurbs/interface/bezierPatch.h,v 1.1 2004/02/02 16:39:08 navaraf Exp $
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
