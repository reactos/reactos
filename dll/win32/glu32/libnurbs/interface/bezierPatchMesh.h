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
** $Date$ $Revision: 1.1 $
*/
/*
** $Header: /cygdrive/c/RCVS/CVS/ReactOS/reactos/lib/glu32/libnurbs/interface/bezierPatchMesh.h,v 1.1 2004/02/02 16:39:08 navaraf Exp $
*/

#ifndef _BEZIERPATCHMESH_H
#define _BEZIERPATCHMESH_H

#include "bezierPatch.h"

typedef struct bezierPatchMesh{
  bezierPatch *bpatch; /*vertex*/
  bezierPatch *bpatch_normal; 
  bezierPatch *bpatch_texcoord; /*s,t,r,q*/
  bezierPatch *bpatch_color; /*RGBA*/

  float *UVarray; /*all UV components of all vertices of all strips*/
  int   *length_array; /*[i] is the number of vertices in the ith strip*/
  GLenum *type_array;  /*[i] is the type of the ith primitive*/

  /*to support dynamic insertion*/
  int size_UVarray;
  int index_UVarray;
  int size_length_array;
  int index_length_array;
  
  int counter; /*track the current strip size*/
  GLenum type; /*track the current type: 0: GL_TRIANGLES, 1: GL_TRIANGLE_STRIP*/
  
  /*we eventually want to evaluate from (u,v) to (x,y,z) and draw them*/
  float *vertex_array; /*each vertex contains three components*/
  float *normal_array; /*each normal contains three components*/
  float *color_array;
  float *texcoord_array;

  /*in case we need a linked list*/
  struct bezierPatchMesh *next;
} bezierPatchMesh;

#ifdef __cplusplus
extern "C" {
#endif



bezierPatchMesh *bezierPatchMeshMake(int maptype, float umin, float umax, int ustride, int uorder, float vmin, float vmax, int vstride, int vorder, float *ctlpoints,  int size_UVarray, int size_length_array);

/*initilize patches to be null*/
bezierPatchMesh *bezierPatchMeshMake2(int size_UVarray, int size_length_array);

void bezierPatchMeshPutPatch(bezierPatchMesh *bpm, int maptype, float umin, float umax, int ustride, int uorder, float vmin, float vmax, int vstride, int vorder, float *ctlpoints);

void bezierPatchMeshDelete(bezierPatchMesh *bpm);

void bezierPatchMeshBeginStrip(bezierPatchMesh *bpm, GLenum type);

void bezierPatchMeshEndStrip(bezierPatchMesh *bpm);

void bezierPatchMeshInsertUV(bezierPatchMesh *bpm, float u, float v);

void bezierPatchMeshPrint(bezierPatchMesh *bpm);

bezierPatchMesh* bezierPatchMeshListInsert(bezierPatchMesh* list, bezierPatchMesh* bpm);

void bezierPatchMeshListPrint(bezierPatchMesh* list);

int bezierPatchMeshListTotalStrips(bezierPatchMesh* list);

int bezierPatchMeshListTotalVert(bezierPatchMesh* list);
int bezierPatchMeshNumTriangles(bezierPatchMesh* bpm);
int bezierPatchMeshListNumTriangles(bezierPatchMesh* list);

void bezierPatchMeshDelDeg(bezierPatchMesh* bpm);


void bezierPatchMeshEval(bezierPatchMesh* bpm);
  
void bezierPatchMeshDraw(bezierPatchMesh* bpm);

void bezierPatchMeshListDraw(bezierPatchMesh* list);
void bezierPatchMeshListEval(bezierPatchMesh* list);
void bezierPatchMeshListCollect(bezierPatchMesh* list, float **vertex_array, float **normal_array, int **length_array, GLenum **type_array, int *num_strips);

void bezierPatchMeshListDelDeg(bezierPatchMesh* list);
void bezierPatchMeshListDelete(bezierPatchMesh *list);
bezierPatchMesh* bezierPatchMeshListReverse(bezierPatchMesh* list);
void drawStrips(float *vertex_array, float *normal_array, int *length_array, GLenum *type_array, int num_strips);

#ifdef __cplusplus
}
#endif

#endif
