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
*/
/*
*/

#include "gluos.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <GL/glu.h> /*for drawing bzier patch*/
#include "bezierPatch.h"
#include "bezierEval.h"

/*
 *allocate an instance of bezierPatch. The control points are unknown. But
 *the space of this array is allocated with size of 
 *   uorder*vorder*dimension
 *
 */
bezierPatch* bezierPatchMake(float umin, float vmin, float umax, float vmax, int uorder, int vorder, int dimension)
{
  bezierPatch* ret = (bezierPatch*) malloc(sizeof(bezierPatch));
  assert(ret);
  ret->umin = umin;
  ret->vmin = vmin;
  ret->umax = umax;
  ret->vmax = vmax;
  ret->uorder = uorder;
  ret->vorder = vorder;
  ret->dimension = dimension;
  ret->ctlpoints = (float*) malloc(sizeof(float) * dimension * uorder * vorder);
  assert(ret->ctlpoints);

  ret->next = NULL;

  return ret;
}

bezierPatch* bezierPatchMake2(float umin, float vmin, float umax, float vmax, int uorder, int vorder, int dimension, int ustride, int vstride,  float* ctlpoints)
{
  bezierPatch* ret = (bezierPatch*) malloc(sizeof(bezierPatch));
  assert(ret);
  ret->umin = umin;
  ret->vmin = vmin;
  ret->umax = umax;
  ret->vmax = vmax;
  ret->uorder = uorder;
  ret->vorder = vorder;
  ret->dimension = dimension;
  ret->ctlpoints = (float*) malloc(sizeof(float) * dimension * uorder * vorder);
  assert(ret->ctlpoints);

  /*copy the control points there*/
  int the_ustride = vorder * dimension;
  int the_vstride = dimension;
  for(int i=0; i<uorder; i++)
    for(int j=0; j<vorder; j++)
      for(int k=0; k<dimension; k++)
	ret->ctlpoints[i * the_ustride + j*the_vstride+k] = ctlpoints[i*ustride+j*vstride+k];
  
  ret->next = NULL;

  return ret;
}

/*
 *deallocate the space as allocated by Make
 */
void bezierPatchDelete(bezierPatch *b)
{
  free(b->ctlpoints);
  free(b);
}

/*delete the whole linked list
 */
void bezierPatchDeleteList(bezierPatch *b)
{
  bezierPatch *temp;
  while (b != NULL) {
    temp = b;
    b = b->next;
    bezierPatchDelete(temp);    
  }
}

bezierPatch* bezierPatchInsert(bezierPatch *list, bezierPatch *b)
{
  b->next = list;
  return b;
}

/*print the data stored in this patch*/
void bezierPatchPrint(bezierPatch *b)
{
  printf("bezierPatch:\n");
  printf("umin,umax=(%f,%f), (vmin, vmax)=(%f,%f)\n", b->umin, b->umax, b->vmin, b->vmax);
  printf("uorder=%i, vorder=%i\n", b->uorder, b->vorder);
  printf("idmension = %i\n", b->dimension);
}
       
/*print the whole list*/
void bezierPatchPrintList(bezierPatch *list)
{
  bezierPatch* temp;
  for(temp=list; temp != NULL; temp = temp->next)
    bezierPatchPrint(temp);
}

void bezierPatchEval(bezierPatch *b, float u, float v, float ret[])
{
  if(   u >= b->umin && u<= b->umax
     && v >= b->vmin && v<= b->vmax)
    {

      bezierSurfEval(b->umin, b->umax, b->uorder, b->vmin, b->vmax, b->vorder, b->dimension, b->ctlpoints, b->dimension * b->vorder, b->dimension, u, v, ret);

     }
  else if(b->next != NULL)
    bezierPatchEval(b->next, u,v, ret);
  else 
    bezierSurfEval(b->umin, b->umax, b->uorder, b->vmin, b->vmax, b->vorder, b->dimension, b->ctlpoints, b->dimension * b->vorder, b->dimension, u, v, ret);    
}

/*the returned normal is normlized
 */
void bezierPatchEvalNormal(bezierPatch *b, float u, float v, float ret[])
{
  bezierSurfEvalNormal(b->umin, b->umax, b->uorder, b->vmin, b->vmax, b->vorder, b->dimension, b->ctlpoints, b->dimension * b->vorder, b->dimension, u, v, ret);  

  if(   u >= b->umin && u<= b->umax
     && v >= b->vmin && v<= b->vmax)
    {
      bezierSurfEvalNormal(b->umin, b->umax, b->uorder, b->vmin, b->vmax, b->vorder, b->dimension, b->ctlpoints, b->dimension * b->vorder, b->dimension, u, v, ret);
     }
  else if(b->next != NULL)
    bezierPatchEvalNormal(b->next, u,v, ret);
  else 
    bezierSurfEvalNormal(b->umin, b->umax, b->uorder, b->vmin, b->vmax, b->vorder, b->dimension, b->ctlpoints, b->dimension * b->vorder, b->dimension, u, v, ret);    

}

void bezierPatchDraw(bezierPatch *bpatch, int u_reso, int v_reso)
{
  if(bpatch->dimension == 3)
    glMap2f(GL_MAP2_VERTEX_3, bpatch->umin, bpatch->umax, 3*bpatch->vorder, bpatch->uorder, bpatch->vmin, bpatch->vmax,3, bpatch->vorder, (GLfloat*) bpatch->ctlpoints);
  else
    glMap2f(GL_MAP2_VERTEX_4, bpatch->umin, bpatch->umax, 4*bpatch->vorder, bpatch->uorder, bpatch->vmin, bpatch->vmax,3, bpatch->vorder, (GLfloat*) bpatch->ctlpoints);    

  glMapGrid2f(u_reso, bpatch->umin, bpatch->umax, 
	      v_reso, bpatch->vmin, bpatch->vmax);
  glEvalMesh2(GL_LINE, 0, u_reso, 0, v_reso);
}

void bezierPatchListDraw(bezierPatch *list, int u_reso, int v_reso)
{
  bezierPatch *temp;
glEnable(GL_LIGHTING);
glEnable(GL_LIGHT0);
glEnable(GL_MAP2_VERTEX_3);
glEnable(GL_AUTO_NORMAL);
glEnable(GL_NORMALIZE);
glColor3f(1,0,0);
#ifdef DEBUG
printf("mapmap\n");
#endif


  for(temp = list; temp != NULL; temp = temp->next)
    bezierPatchDraw(temp, u_reso, v_reso);
}
	    


