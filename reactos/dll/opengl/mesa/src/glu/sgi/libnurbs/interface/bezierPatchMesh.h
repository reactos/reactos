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

#ifndef _BEZIERPATCHMESH_H
#define _BEZIERPATCHMESH_H

#include <GL/gl.h>
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
