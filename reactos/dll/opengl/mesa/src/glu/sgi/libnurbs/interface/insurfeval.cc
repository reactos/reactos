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
#include <GL/gl.h>
#include <math.h>
#include <assert.h>

#include "glsurfeval.h"

//extern int surfcount;

//#define CRACK_TEST

#define AVOID_ZERO_NORMAL

#ifdef AVOID_ZERO_NORMAL
#define myabs(x)  ((x>0)? x: (-x))
#define MYZERO 0.000001
#define MYDELTA 0.001
#endif

//#define USE_LOD
#ifdef USE_LOD
//#define LOD_EVAL_COORD(u,v) inDoEvalCoord2EM(u,v)
#define LOD_EVAL_COORD(u,v) glEvalCoord2f(u,v)

static void LOD_interpolate(REAL A[2], REAL B[2], REAL C[2], int j, int k, int pow2_level,
			    REAL& u, REAL& v)
{
  REAL a,a1,b,b1;

  a = ((REAL) j) / ((REAL) pow2_level);
  a1 = 1-a;

  if(j != 0)
    {
      b = ((REAL) k) / ((REAL)j);
      b1 = 1-b;
    }
  REAL x,y,z;
  x = a1;
  if(j==0)
    {
      y=0; z=0;
    }
  else{
    y = b1*a;
    z = b *a;
  }

  u = x*A[0] + y*B[0] + z*C[0];
  v = x*A[1] + y*B[1] + z*C[1];
}

void OpenGLSurfaceEvaluator::LOD_triangle(REAL A[2], REAL B[2], REAL C[2], 
			 int level)     
{
  int k,j;
  int pow2_level;
  /*compute 2^level*/
  pow2_level = 1;

  for(j=0; j<level; j++)
    pow2_level *= 2;
  for(j=0; j<=pow2_level-1; j++)
    {
      REAL u,v;

/*      beginCallBack(GL_TRIANGLE_STRIP);*/
glBegin(GL_TRIANGLE_STRIP);
      LOD_interpolate(A,B,C, j+1, j+1, pow2_level, u,v);
#ifdef USE_LOD
      LOD_EVAL_COORD(u,v);
//      glEvalCoord2f(u,v);
#else
      inDoEvalCoord2EM(u,v);
#endif

      for(k=0; k<=j; k++)
	{
	  LOD_interpolate(A,B,C,j,j-k,pow2_level, u,v);
#ifdef USE_LOD
          LOD_EVAL_COORD(u,v);
//	  glEvalCoord2f(u,v);
#else
	  inDoEvalCoord2EM(u,v);
#endif

	  LOD_interpolate(A,B,C,j+1,j-k,pow2_level, u,v);

#ifdef USE_LOD
	  LOD_EVAL_COORD(u,v);
//	  glEvalCoord2f(u,v);
#else
	  inDoEvalCoord2EM(u,v);
#endif
	}
//      endCallBack();	
glEnd();
    }
}

void OpenGLSurfaceEvaluator::LOD_eval(int num_vert, REAL* verts, int type,
		     int level
		     )
{
  int i,k;
  switch(type){
  case GL_TRIANGLE_STRIP:
  case GL_QUAD_STRIP:
    for(i=2, k=4; i<=num_vert-2; i+=2, k+=4)
      {
	LOD_triangle(verts+k-4, verts+k-2, verts+k,
		     level
		     );
	LOD_triangle(verts+k-2, verts+k+2, verts+k,
		     level
		     );
      }
    if(num_vert % 2 ==1) 
      {
	LOD_triangle(verts+2*(num_vert-3), verts+2*(num_vert-2), verts+2*(num_vert-1),
		     level
		     );
      }
    break;      
  case GL_TRIANGLE_FAN:
    for(i=1, k=2; i<=num_vert-2; i++, k+=2)
      {
	LOD_triangle(verts,verts+k, verts+k+2,
		     level
		     );
      }
    break;
  
  default:
    fprintf(stderr, "typy not supported in LOD_\n");
  }
}
	

#endif //USE_LOD

//#define  GENERIC_TEST
#ifdef GENERIC_TEST
extern float xmin, xmax, ymin, ymax, zmin, zmax; /*bounding box*/
extern int temp_signal;

static void gTessVertexSphere(float u, float v, float temp_normal[3], float temp_vertex[3])
{
  float r=2.0;
  float Ox = 0.5*(xmin+xmax);
  float Oy = 0.5*(ymin+ymax);
  float Oz = 0.5*(zmin+zmax);
  float nx = cos(v) * sin(u);
  float ny = sin(v) * sin(u);
  float nz = cos(u);
  float x= Ox+r * nx;
  float y= Oy+r * ny;
  float z= Oz+r * nz;

  temp_normal[0] = nx;
  temp_normal[1] = ny;
  temp_normal[2] =  nz;
  temp_vertex[0] = x;
  temp_vertex[1] = y;
  temp_vertex[2] = z;

//  glNormal3f(nx,ny,nz);
//  glVertex3f(x,y,z);
}

static void gTessVertexCyl(float u, float v, float temp_normal[3], float temp_vertex[3])
{
   float r=2.0;
  float Ox = 0.5*(xmin+xmax);
  float Oy = 0.5*(ymin+ymax);
  float Oz = 0.5*(zmin+zmax);
  float nx = cos(v);
  float ny = sin(v);
  float nz = 0;
  float x= Ox+r * nx;
  float y= Oy+r * ny;
  float z= Oz - 2*u;

  temp_normal[0] = nx;
  temp_normal[1] = ny;
  temp_normal[2] =  nz;
  temp_vertex[0] = x;
  temp_vertex[1] = y;
  temp_vertex[2] = z;

/*  
  glNormal3f(nx,ny,nz);
  glVertex3f(x,y,z);
*/
}

#endif //GENERIC_TEST

void OpenGLSurfaceEvaluator::inBPMListEval(bezierPatchMesh* list)
{
  bezierPatchMesh* temp;
  for(temp = list; temp != NULL; temp = temp->next)
    {
      inBPMEval(temp);
    }
}

void OpenGLSurfaceEvaluator::inBPMEval(bezierPatchMesh* bpm)
{
  int i,j,k,l;
  float u,v;

  int ustride = bpm->bpatch->dimension * bpm->bpatch->vorder;
  int vstride = bpm->bpatch->dimension;
  inMap2f( 
	  (bpm->bpatch->dimension == 3)? GL_MAP2_VERTEX_3 : GL_MAP2_VERTEX_4,
	  bpm->bpatch->umin,
	  bpm->bpatch->umax,
	  ustride,
	  bpm->bpatch->uorder,
	  bpm->bpatch->vmin,
	  bpm->bpatch->vmax,
	  vstride,
	  bpm->bpatch->vorder,
	  bpm->bpatch->ctlpoints);
  
  bpm->vertex_array = (float*) malloc(sizeof(float)* (bpm->index_UVarray/2) * 3+1); /*in case the origional dimenion is 4, then we need 4 space to pass to evaluator.*/
  assert(bpm->vertex_array);
  bpm->normal_array = (float*) malloc(sizeof(float)* (bpm->index_UVarray/2) * 3);
  assert(bpm->normal_array);
#ifdef CRACK_TEST
if(  global_ev_u1 ==2 &&   global_ev_u2 == 3
  && global_ev_v1 ==2 &&   global_ev_v2 == 3)
{
REAL vertex[4];
REAL normal[4];
#ifdef DEBUG
printf("***number 1\n");
#endif

beginCallBack(GL_QUAD_STRIP, NULL);
inEvalCoord2f(3.0, 3.0);
inEvalCoord2f(2.0, 3.0);
inEvalCoord2f(3.0, 2.7);
inEvalCoord2f(2.0, 2.7);
inEvalCoord2f(3.0, 2.0);
inEvalCoord2f(2.0, 2.0);
endCallBack(NULL);


beginCallBack(GL_TRIANGLE_STRIP, NULL);
inEvalCoord2f(2.0, 3.0);
inEvalCoord2f(2.0, 2.0);
inEvalCoord2f(2.0, 2.7);
endCallBack(NULL);

}

/*
if(  global_ev_u1 ==2 &&   global_ev_u2 == 3
  && global_ev_v1 ==1 &&   global_ev_v2 == 2)
{
#ifdef DEBUG
printf("***number 2\n");
#endif
beginCallBack(GL_QUAD_STRIP);
inEvalCoord2f(2.0, 2.0);
inEvalCoord2f(2.0, 1.0);
inEvalCoord2f(3.0, 2.0);
inEvalCoord2f(3.0, 1.0);
endCallBack();
}
*/
if(  global_ev_u1 ==1 &&   global_ev_u2 == 2
  && global_ev_v1 ==2 &&   global_ev_v2 == 3)
{
#ifdef DEBUG
printf("***number 3\n");
#endif
beginCallBack(GL_QUAD_STRIP, NULL);
inEvalCoord2f(2.0, 3.0);
inEvalCoord2f(1.0, 3.0);
inEvalCoord2f(2.0, 2.3);
inEvalCoord2f(1.0, 2.3);
inEvalCoord2f(2.0, 2.0);
inEvalCoord2f(1.0, 2.0);
endCallBack(NULL);

beginCallBack(GL_TRIANGLE_STRIP, NULL);
inEvalCoord2f(2.0, 2.3);
inEvalCoord2f(2.0, 2.0);
inEvalCoord2f(2.0, 3.0);
endCallBack(NULL);

}
return;
#endif

  k=0;
  l=0;

  for(i=0; i<bpm->index_length_array; i++)
    {
      beginCallBack(bpm->type_array[i], userData);
      for(j=0; j<bpm->length_array[i]; j++)
	{
	  u = bpm->UVarray[k];
	  v = bpm->UVarray[k+1];
	  inDoEvalCoord2NOGE(u,v,
			     bpm->vertex_array+l,
			     bpm->normal_array+l);

	  normalCallBack(bpm->normal_array+l, userData);
	  vertexCallBack(bpm->vertex_array+l, userData);

	  k += 2;
	  l += 3;
	}
      endCallBack(userData);
    }
}

void OpenGLSurfaceEvaluator::inEvalPoint2(int i, int j)
{
  REAL du, dv;
  REAL point[4];
  REAL normal[3];
  REAL u,v;
  du = (global_grid_u1 - global_grid_u0) / (REAL)global_grid_nu;
  dv = (global_grid_v1 - global_grid_v0) / (REAL)global_grid_nv;
  u = (i==global_grid_nu)? global_grid_u1:(global_grid_u0 + i*du);
  v = (j == global_grid_nv)? global_grid_v1: (global_grid_v0 +j*dv);
  inDoEvalCoord2(u,v,point,normal);
}

void OpenGLSurfaceEvaluator::inEvalCoord2f(REAL u, REAL v)
{

  REAL point[4];
  REAL normal[3];
  inDoEvalCoord2(u,v,point, normal);
}



/*define a grid. store the values into the global variabls:
 * global_grid_*
 *These values will be used later by evaluating functions
 */
void OpenGLSurfaceEvaluator::inMapGrid2f(int nu, REAL u0, REAL u1,
		 int nv, REAL v0, REAL v1)
{
 global_grid_u0 = u0;
 global_grid_u1 = u1;
 global_grid_nu = nu;
 global_grid_v0 = v0;
 global_grid_v1 = v1;
 global_grid_nv = nv;
}

void OpenGLSurfaceEvaluator::inEvalMesh2(int lowU, int lowV, int highU, int highV)
{
  REAL du, dv;
  int i,j;
  REAL point[4];
  REAL normal[3];
  if(global_grid_nu == 0 || global_grid_nv == 0)
    return; /*no points need to be output*/
  du = (global_grid_u1 - global_grid_u0) / (REAL)global_grid_nu;
  dv = (global_grid_v1 - global_grid_v0) / (REAL)global_grid_nv;  
  
  if(global_grid_nu >= global_grid_nv){
    for(i=lowU; i<highU; i++){
      REAL u1 = (i==global_grid_nu)? global_grid_u1:(global_grid_u0 + i*du);
      REAL u2 = ((i+1) == global_grid_nu)? global_grid_u1: (global_grid_u0+(i+1)*du);
      
      bgnqstrip();
      for(j=highV; j>=lowV; j--){
	REAL v1 = (j == global_grid_nv)? global_grid_v1: (global_grid_v0 +j*dv);
	
	inDoEvalCoord2(u1, v1, point, normal);
	inDoEvalCoord2(u2, v1, point, normal);
      }
      endqstrip();
    }
  }
  
  else{
    for(i=lowV; i<highV; i++){
      REAL v1 = (i==global_grid_nv)? global_grid_v1:(global_grid_v0 + i*dv);
      REAL v2 = ((i+1) == global_grid_nv)? global_grid_v1: (global_grid_v0+(i+1)*dv);
      
      bgnqstrip();
      for(j=highU; j>=lowU; j--){
	REAL u1 = (j == global_grid_nu)? global_grid_u1: (global_grid_u0 +j*du);	
	inDoEvalCoord2(u1, v2, point, normal);
	inDoEvalCoord2(u1, v1, point, normal);
      }
      endqstrip();
    }
  }
    
}

void OpenGLSurfaceEvaluator::inMap2f(int k,
	     REAL ulower,
	     REAL uupper,
	     int ustride,
	     int uorder,
	     REAL vlower,
	     REAL vupper,
	     int vstride,
	     int vorder,
	     REAL *ctlPoints)
{
  int i,j,x;
  REAL *data = global_ev_ctlPoints;
  


  if(k == GL_MAP2_VERTEX_3) k=3;
  else if (k==GL_MAP2_VERTEX_4) k =4;
  else {
    printf("error in inMap2f, maptype=%i is wrong, k,map is not updated\n", k);
    return;
  }
  
  global_ev_k = k;
  global_ev_u1 = ulower;
  global_ev_u2 = uupper;
  global_ev_ustride = ustride;
  global_ev_uorder = uorder;
  global_ev_v1 = vlower;
  global_ev_v2 = vupper;
  global_ev_vstride = vstride;
  global_ev_vorder = vorder;

  /*copy the contrl points from ctlPoints to global_ev_ctlPoints*/
  for (i=0; i<uorder; i++) {
    for (j=0; j<vorder; j++) {
      for (x=0; x<k; x++) {
	data[x] = ctlPoints[x];
      }
      ctlPoints += vstride;
      data += k;
    }
    ctlPoints += ustride - vstride * vorder;
  }

}


/*
 *given a point p with homegeneous coordiante (x,y,z,w), 
 *let pu(x,y,z,w) be its partial derivative vector with
 *respect to u
 *and pv(x,y,z,w) be its partial derivative vector with repect to v.
 *This function returns the partial derivative vectors of the
 *inhomegensous coordinates, i.e., 
 * (x/w, y/w, z/w) with respect to u and v.
 */
void OpenGLSurfaceEvaluator::inComputeFirstPartials(REAL *p, REAL *pu, REAL *pv)
{
    pu[0] = pu[0]*p[3] - pu[3]*p[0];
    pu[1] = pu[1]*p[3] - pu[3]*p[1];
    pu[2] = pu[2]*p[3] - pu[3]*p[2];

    pv[0] = pv[0]*p[3] - pv[3]*p[0];
    pv[1] = pv[1]*p[3] - pv[3]*p[1];
    pv[2] = pv[2]*p[3] - pv[3]*p[2];
}

/*compute the cross product of pu and pv and normalize.
 *the normal is returned in retNormal
 * pu: dimension 3
 * pv: dimension 3
 * n: return normal, of dimension 3
 */
void OpenGLSurfaceEvaluator::inComputeNormal2(REAL *pu, REAL *pv, REAL *n)
{
  REAL mag; 

  n[0] = pu[1]*pv[2] - pu[2]*pv[1];
  n[1] = pu[2]*pv[0] - pu[0]*pv[2];
  n[2] = pu[0]*pv[1] - pu[1]*pv[0];  

  mag = sqrt(n[0]*n[0] + n[1]*n[1] + n[2]*n[2]);

  if (mag > 0.0) {
     n[0] /= mag; 
     n[1] /= mag;
     n[2] /= mag;
  }
}
 


/*Compute point and normal
 *see the head of inDoDomain2WithDerivs
 *for the meaning of the arguments
 */
void OpenGLSurfaceEvaluator::inDoEvalCoord2(REAL u, REAL v,
			   REAL *retPoint, REAL *retNormal)
{

  REAL du[4];
  REAL dv[4];

 
  assert(global_ev_k>=3 && global_ev_k <= 4);
  /*compute homegeneous point and partial derivatives*/
  inDoDomain2WithDerivs(global_ev_k, u, v, global_ev_u1, global_ev_u2, global_ev_uorder, global_ev_v1, global_ev_v2, global_ev_vorder, global_ev_ctlPoints, retPoint, du, dv);

#ifdef AVOID_ZERO_NORMAL

  if(myabs(dv[0]) <= MYZERO && myabs(dv[1]) <= MYZERO && myabs(dv[2]) <= MYZERO)
    {

      REAL tempdu[4];
      REAL tempdata[4];
      REAL u1 = global_ev_u1;
      REAL u2 = global_ev_u2;
      if(u-MYDELTA*(u2-u1) < u1)
	u = u+ MYDELTA*(u2-u1);
      else
	u = u-MYDELTA*(u2-u1);
      inDoDomain2WithDerivs(global_ev_k, u,v,global_ev_u1, global_ev_u2, global_ev_uorder, global_ev_v1, global_ev_v2, global_ev_vorder, global_ev_ctlPoints, tempdata, tempdu, dv);
    }
  if(myabs(du[0]) <= MYZERO && myabs(du[1]) <= MYZERO && myabs(du[2]) <= MYZERO)
    {
      REAL tempdv[4];
      REAL tempdata[4];
      REAL v1 = global_ev_v1;
      REAL v2 = global_ev_v2;
      if(v-MYDELTA*(v2-v1) < v1)
	v = v+ MYDELTA*(v2-v1);
      else
	v = v-MYDELTA*(v2-v1);
      inDoDomain2WithDerivs(global_ev_k, u,v,global_ev_u1, global_ev_u2, global_ev_uorder, global_ev_v1, global_ev_v2, global_ev_vorder, global_ev_ctlPoints, tempdata, du, tempdv);
    }
#endif


  /*compute normal*/
  switch(global_ev_k){
  case 3:
    inComputeNormal2(du, dv, retNormal);

    break;
  case 4:
    inComputeFirstPartials(retPoint, du, dv);
    inComputeNormal2(du, dv, retNormal);
    /*transform the homegeneous coordinate of retPoint into inhomogenous one*/
    retPoint[0] /= retPoint[3];
    retPoint[1] /= retPoint[3];
    retPoint[2] /= retPoint[3];
    break;
  }
  /*output this vertex*/
/*  inMeshStreamInsert(global_ms, retPoint, retNormal);*/



  glNormal3fv(retNormal);
  glVertex3fv(retPoint);




  #ifdef DEBUG
  printf("vertex(%f,%f,%f)\n", retPoint[0],retPoint[1],retPoint[2]);
  #endif
  


}

/*Compute point and normal
 *see the head of inDoDomain2WithDerivs
 *for the meaning of the arguments
 */
void OpenGLSurfaceEvaluator::inDoEvalCoord2NOGE_BU(REAL u, REAL v,
			   REAL *retPoint, REAL *retNormal)
{

  REAL du[4];
  REAL dv[4];

 
  assert(global_ev_k>=3 && global_ev_k <= 4);
  /*compute homegeneous point and partial derivatives*/
//   inPreEvaluateBU(global_ev_k, global_ev_uorder, global_ev_vorder, (u-global_ev_u1)/(global_ev_u2-global_ev_u1), global_ev_ctlPoints);
  inDoDomain2WithDerivsBU(global_ev_k, u, v, global_ev_u1, global_ev_u2, global_ev_uorder, global_ev_v1, global_ev_v2, global_ev_vorder, global_ev_ctlPoints, retPoint, du, dv);


#ifdef AVOID_ZERO_NORMAL

  if(myabs(dv[0]) <= MYZERO && myabs(dv[1]) <= MYZERO && myabs(dv[2]) <= MYZERO)
    {

      REAL tempdu[4];
      REAL tempdata[4];
      REAL u1 = global_ev_u1;
      REAL u2 = global_ev_u2;
      if(u-MYDELTA*(u2-u1) < u1)
	u = u+ MYDELTA*(u2-u1);
      else
	u = u-MYDELTA*(u2-u1);
      inDoDomain2WithDerivs(global_ev_k, u,v,global_ev_u1, global_ev_u2, global_ev_uorder, global_ev_v1, global_ev_v2, global_ev_vorder, global_ev_ctlPoints, tempdata, tempdu, dv);
    }
  if(myabs(du[0]) <= MYZERO && myabs(du[1]) <= MYZERO && myabs(du[2]) <= MYZERO)
    {
      REAL tempdv[4];
      REAL tempdata[4];
      REAL v1 = global_ev_v1;
      REAL v2 = global_ev_v2;
      if(v-MYDELTA*(v2-v1) < v1)
	v = v+ MYDELTA*(v2-v1);
      else
	v = v-MYDELTA*(v2-v1);
      inDoDomain2WithDerivs(global_ev_k, u,v,global_ev_u1, global_ev_u2, global_ev_uorder, global_ev_v1, global_ev_v2, global_ev_vorder, global_ev_ctlPoints, tempdata, du, tempdv);
    }
#endif

  /*compute normal*/
  switch(global_ev_k){
  case 3:
    inComputeNormal2(du, dv, retNormal);
    break;
  case 4:
    inComputeFirstPartials(retPoint, du, dv);
    inComputeNormal2(du, dv, retNormal);
    /*transform the homegeneous coordinate of retPoint into inhomogenous one*/
    retPoint[0] /= retPoint[3];
    retPoint[1] /= retPoint[3];
    retPoint[2] /= retPoint[3];
    break;
  }
}

/*Compute point and normal
 *see the head of inDoDomain2WithDerivs
 *for the meaning of the arguments
 */
void OpenGLSurfaceEvaluator::inDoEvalCoord2NOGE_BV(REAL u, REAL v,
			   REAL *retPoint, REAL *retNormal)
{

  REAL du[4];
  REAL dv[4];

 
  assert(global_ev_k>=3 && global_ev_k <= 4);
  /*compute homegeneous point and partial derivatives*/
//   inPreEvaluateBV(global_ev_k, global_ev_uorder, global_ev_vorder, (v-global_ev_v1)/(global_ev_v2-global_ev_v1), global_ev_ctlPoints);

  inDoDomain2WithDerivsBV(global_ev_k, u, v, global_ev_u1, global_ev_u2, global_ev_uorder, global_ev_v1, global_ev_v2, global_ev_vorder, global_ev_ctlPoints, retPoint, du, dv);


#ifdef AVOID_ZERO_NORMAL

  if(myabs(dv[0]) <= MYZERO && myabs(dv[1]) <= MYZERO && myabs(dv[2]) <= MYZERO)
    {

      REAL tempdu[4];
      REAL tempdata[4];
      REAL u1 = global_ev_u1;
      REAL u2 = global_ev_u2;
      if(u-MYDELTA*(u2-u1) < u1)
	u = u+ MYDELTA*(u2-u1);
      else
	u = u-MYDELTA*(u2-u1);
      inDoDomain2WithDerivs(global_ev_k, u,v,global_ev_u1, global_ev_u2, global_ev_uorder, global_ev_v1, global_ev_v2, global_ev_vorder, global_ev_ctlPoints, tempdata, tempdu, dv);
    }
  if(myabs(du[0]) <= MYZERO && myabs(du[1]) <= MYZERO && myabs(du[2]) <= MYZERO)
    {
      REAL tempdv[4];
      REAL tempdata[4];
      REAL v1 = global_ev_v1;
      REAL v2 = global_ev_v2;
      if(v-MYDELTA*(v2-v1) < v1)
	v = v+ MYDELTA*(v2-v1);
      else
	v = v-MYDELTA*(v2-v1);
      inDoDomain2WithDerivs(global_ev_k, u,v,global_ev_u1, global_ev_u2, global_ev_uorder, global_ev_v1, global_ev_v2, global_ev_vorder, global_ev_ctlPoints, tempdata, du, tempdv);
    }
#endif

  /*compute normal*/
  switch(global_ev_k){
  case 3:
    inComputeNormal2(du, dv, retNormal);
    break;
  case 4:
    inComputeFirstPartials(retPoint, du, dv);
    inComputeNormal2(du, dv, retNormal);
    /*transform the homegeneous coordinate of retPoint into inhomogenous one*/
    retPoint[0] /= retPoint[3];
    retPoint[1] /= retPoint[3];
    retPoint[2] /= retPoint[3];
    break;
  }
}
 

/*Compute point and normal
 *see the head of inDoDomain2WithDerivs
 *for the meaning of the arguments
 */
void OpenGLSurfaceEvaluator::inDoEvalCoord2NOGE(REAL u, REAL v,
			   REAL *retPoint, REAL *retNormal)
{

  REAL du[4];
  REAL dv[4];

 
  assert(global_ev_k>=3 && global_ev_k <= 4);
  /*compute homegeneous point and partial derivatives*/
  inDoDomain2WithDerivs(global_ev_k, u, v, global_ev_u1, global_ev_u2, global_ev_uorder, global_ev_v1, global_ev_v2, global_ev_vorder, global_ev_ctlPoints, retPoint, du, dv);


#ifdef AVOID_ZERO_NORMAL

  if(myabs(dv[0]) <= MYZERO && myabs(dv[1]) <= MYZERO && myabs(dv[2]) <= MYZERO)
    {

      REAL tempdu[4];
      REAL tempdata[4];
      REAL u1 = global_ev_u1;
      REAL u2 = global_ev_u2;
      if(u-MYDELTA*(u2-u1) < u1)
	u = u+ MYDELTA*(u2-u1);
      else
	u = u-MYDELTA*(u2-u1);
      inDoDomain2WithDerivs(global_ev_k, u,v,global_ev_u1, global_ev_u2, global_ev_uorder, global_ev_v1, global_ev_v2, global_ev_vorder, global_ev_ctlPoints, tempdata, tempdu, dv);
    }
  if(myabs(du[0]) <= MYZERO && myabs(du[1]) <= MYZERO && myabs(du[2]) <= MYZERO)
    {
      REAL tempdv[4];
      REAL tempdata[4];
      REAL v1 = global_ev_v1;
      REAL v2 = global_ev_v2;
      if(v-MYDELTA*(v2-v1) < v1)
	v = v+ MYDELTA*(v2-v1);
      else
	v = v-MYDELTA*(v2-v1);
      inDoDomain2WithDerivs(global_ev_k, u,v,global_ev_u1, global_ev_u2, global_ev_uorder, global_ev_v1, global_ev_v2, global_ev_vorder, global_ev_ctlPoints, tempdata, du, tempdv);
    }
#endif

  /*compute normal*/
  switch(global_ev_k){
  case 3:
    inComputeNormal2(du, dv, retNormal);
    break;
  case 4:
    inComputeFirstPartials(retPoint, du, dv);
    inComputeNormal2(du, dv, retNormal);
    /*transform the homegeneous coordinate of retPoint into inhomogenous one*/
    retPoint[0] /= retPoint[3];
    retPoint[1] /= retPoint[3];
    retPoint[2] /= retPoint[3];
    break;
  }
//  glNormal3fv(retNormal);
//  glVertex3fv(retPoint);
}
 
void OpenGLSurfaceEvaluator::inPreEvaluateBV(int k, int uorder, int vorder, REAL vprime, REAL *baseData)
{
  int j,row,col;
  REAL p, pdv;
  REAL *data;

  if(global_vprime != vprime || global_vorder != vorder) {      
    inPreEvaluateWithDeriv(vorder, vprime, global_vcoeff, global_vcoeffDeriv);
    global_vprime = vprime;
    global_vorder = vorder;
  }

  for(j=0; j<k; j++){
    data = baseData+j;
    for(row=0; row<uorder; row++){
      p = global_vcoeff[0] * (*data);
      pdv = global_vcoeffDeriv[0] * (*data);
      data += k;
      for(col = 1; col < vorder; col++){
	p += global_vcoeff[col] *  (*data);
	pdv += global_vcoeffDeriv[col] * (*data);
	data += k;
      }
      global_BV[row][j]  = p;
      global_PBV[row][j]  = pdv;
    }
  }
}

void OpenGLSurfaceEvaluator::inPreEvaluateBU(int k, int uorder, int vorder, REAL uprime, REAL *baseData)
{
  int j,row,col;
  REAL p, pdu;
  REAL *data;

  if(global_uprime != uprime || global_uorder != uorder) {      
    inPreEvaluateWithDeriv(uorder, uprime, global_ucoeff, global_ucoeffDeriv);
    global_uprime = uprime;
    global_uorder = uorder;
  }

  for(j=0; j<k; j++){
    data = baseData+j;
    for(col=0; col<vorder; col++){
      data = baseData+j + k*col;
      p = global_ucoeff[0] * (*data);
      pdu = global_ucoeffDeriv[0] * (*data);
      data += k*uorder;
      for(row = 1; row < uorder; row++){
	p += global_ucoeff[row] *  (*data);
	pdu += global_ucoeffDeriv[row] * (*data);
	data += k * uorder;
      }
      global_BU[col][j]  = p;
      global_PBU[col][j]  = pdu;
    }
  }
}
 
void OpenGLSurfaceEvaluator::inDoDomain2WithDerivsBU(int k, REAL u, REAL v,
						      REAL u1, REAL u2, int uorder,
						      REAL v1, REAL v2, int vorder,
						      REAL *baseData,
						      REAL *retPoint, REAL* retdu, REAL *retdv)
{
  int j, col;

  REAL vprime;


  if((u2 == u1) || (v2 == v1))
    return;

  vprime = (v - v1) / (v2 - v1);


  if(global_vprime != vprime || global_vorder != vorder) {
    inPreEvaluateWithDeriv(vorder, vprime, global_vcoeff, global_vcoeffDeriv);
    global_vprime = vprime;
    global_vorder = vorder;
  }


  for(j=0; j<k; j++)
    {
      retPoint[j] = retdu[j] = retdv[j] = 0.0;
      for (col = 0; col < vorder; col++)  {
	retPoint[j] += global_BU[col][j] * global_vcoeff[col];
	retdu[j] += global_PBU[col][j] * global_vcoeff[col];
	retdv[j] += global_BU[col][j] * global_vcoeffDeriv[col];
      }
    }
}    
   
void OpenGLSurfaceEvaluator::inDoDomain2WithDerivsBV(int k, REAL u, REAL v,
						      REAL u1, REAL u2, int uorder,
						      REAL v1, REAL v2, int vorder,
						      REAL *baseData,
						      REAL *retPoint, REAL* retdu, REAL *retdv)
{
  int j, row;
  REAL uprime;


  if((u2 == u1) || (v2 == v1))
    return;
  uprime = (u - u1) / (u2 - u1);


  if(global_uprime != uprime || global_uorder != uorder) {
    inPreEvaluateWithDeriv(uorder, uprime, global_ucoeff, global_ucoeffDeriv);
    global_uprime = uprime;
    global_uorder = uorder;
  }


  for(j=0; j<k; j++)
    {
      retPoint[j] = retdu[j] = retdv[j] = 0.0;
      for (row = 0; row < uorder; row++)  {
	retPoint[j] += global_BV[row][j] * global_ucoeff[row];
	retdu[j] += global_BV[row][j] * global_ucoeffDeriv[row];
	retdv[j] += global_PBV[row][j] * global_ucoeff[row];
      }
    }
}
  

/*
 *given a Bezier surface, and parameter (u,v), compute the point in the object space,
 *and the normal
 *k: the dimension of the object space: usually 2,3,or 4.
 *u,v: the paramter pair.
 *u1,u2,uorder: the Bezier polynomial of u coord is defined on [u1,u2] with order uorder.
 *v1,v2,vorder: the Bezier polynomial of v coord is defined on [v1,v2] with order vorder.
 *baseData: contrl points. arranged as: (u,v,k).
 *retPoint:  the computed point (one point) with dimension k.
 *retdu: the computed partial derivative with respect to u.
 *retdv: the computed partial derivative with respect to v.
 */
void OpenGLSurfaceEvaluator::inDoDomain2WithDerivs(int k, REAL u, REAL v, 
				REAL u1, REAL u2, int uorder, 
				REAL v1,  REAL v2, int vorder, 
				REAL *baseData,
				REAL *retPoint, REAL *retdu, REAL *retdv)
{
    int j, row, col;
    REAL uprime;
    REAL vprime;
    REAL p;
    REAL pdv;
    REAL *data;

    if((u2 == u1) || (v2 == v1))
	return;
    uprime = (u - u1) / (u2 - u1);
    vprime = (v - v1) / (v2 - v1);
    
    /* Compute coefficients for values and derivs */

    /* Use already cached values if possible */
    if(global_uprime != uprime || global_uorder != uorder) {
        inPreEvaluateWithDeriv(uorder, uprime, global_ucoeff, global_ucoeffDeriv);
	global_uorder = uorder;
	global_uprime = uprime;
    }
    if (global_vprime != vprime || 
	  global_vorder != vorder) {
	inPreEvaluateWithDeriv(vorder, vprime, global_vcoeff, global_vcoeffDeriv);
	global_vorder = vorder;
	global_vprime = vprime;
    }

    for (j = 0; j < k; j++) {
	data=baseData+j;
	retPoint[j] = retdu[j] = retdv[j] = 0.0;
	for (row = 0; row < uorder; row++)  {
	    /* 
	    ** Minor optimization.
	    ** The col == 0 part of the loop is extracted so we don't
	    ** have to initialize p and pdv to 0.
	    */
	    p = global_vcoeff[0] * (*data);
	    pdv = global_vcoeffDeriv[0] * (*data);
	    data += k;
	    for (col = 1; col < vorder; col++) {
		/* Incrementally build up p, pdv value */
		p += global_vcoeff[col] * (*data);
		pdv += global_vcoeffDeriv[col] * (*data);
		data += k;
	    }
	    /* Use p, pdv value to incrementally add up r, du, dv */
	    retPoint[j] += global_ucoeff[row] * p;
	    retdu[j] += global_ucoeffDeriv[row] * p;
	    retdv[j] += global_ucoeff[row] * pdv;
	}
    }  
}


/*
 *compute the Bezier polynomials C[n,j](v) for all j at v with 
 *return values stored in coeff[], where 
 *  C[n,j](v) = (n,j) * v^j * (1-v)^(n-j),
 *  j=0,1,2,...,n.
 *order : n+1
 *vprime: v
 *coeff : coeff[j]=C[n,j](v), this array store the returned values.
 *The algorithm is a recursive scheme:
 *   C[0,0]=1;
 *   C[n,j](v) = (1-v)*C[n-1,j](v) + v*C[n-1,j-1](v), n>=1
 *This code is copied from opengl/soft/so_eval.c:PreEvaluate
 */
void OpenGLSurfaceEvaluator::inPreEvaluate(int order, REAL vprime, REAL *coeff)
{
  int i, j;
  REAL oldval, temp;
  REAL oneMinusvprime;
  
  /*
   * Minor optimization
   * Compute orders 1 and 2 outright, and set coeff[0], coeff[1] to
     * their i==1 loop values to avoid the initialization and the i==1 loop.
     */
  if (order == 1) {
    coeff[0] = 1.0;
    return;
  }
  
  oneMinusvprime = 1-vprime;
  coeff[0] = oneMinusvprime;
  coeff[1] = vprime;
  if (order == 2) return;
  
  for (i = 2; i < order; i++) {
    oldval = coeff[0] * vprime;
    coeff[0] = oneMinusvprime * coeff[0];
    for (j = 1; j < i; j++) {
      temp = oldval;
      oldval = coeff[j] * vprime;
	    coeff[j] = temp + oneMinusvprime * coeff[j];
    }
    coeff[j] = oldval;
  }
}

/*
 *compute the Bezier polynomials C[n,j](v) and derivatives for all j at v with 
 *return values stored in coeff[] and coeffDeriv[].
 *see the head of function inPreEvaluate for the definition of C[n,j](v)
 *and how to compute the values. 
 *The algorithm to compute the derivative is:
 *   dC[0,0](v) = 0.
 *   dC[n,j](v) = n*(dC[n-1,j-1](v) - dC[n-1,j](v)).
 *
 *This code is copied from opengl/soft/so_eval.c:PreEvaluateWidthDeriv
 */
void OpenGLSurfaceEvaluator::inPreEvaluateWithDeriv(int order, REAL vprime, 
    REAL *coeff, REAL *coeffDeriv)
{
  int i, j;
  REAL oldval, temp;
  REAL oneMinusvprime;
  
  oneMinusvprime = 1-vprime;
  /*
   * Minor optimization
   * Compute orders 1 and 2 outright, and set coeff[0], coeff[1] to 
   * their i==1 loop values to avoid the initialization and the i==1 loop.
   */
  if (order == 1) {
    coeff[0] = 1.0;
    coeffDeriv[0] = 0.0;
    return;
  } else if (order == 2) {
    coeffDeriv[0] = -1.0;
    coeffDeriv[1] = 1.0;
    coeff[0] = oneMinusvprime;
    coeff[1] = vprime;
    return;
  }
  coeff[0] = oneMinusvprime;
  coeff[1] = vprime;
  for (i = 2; i < order - 1; i++) {
    oldval = coeff[0] * vprime;
    coeff[0] = oneMinusvprime * coeff[0];
    for (j = 1; j < i; j++) {
      temp = oldval;
      oldval = coeff[j] * vprime;
      coeff[j] = temp + oneMinusvprime * coeff[j];
    }
    coeff[j] = oldval;
  }
  coeffDeriv[0] = -coeff[0];
  /*
   ** Minor optimization:
   ** Would make this a "for (j=1; j<order-1; j++)" loop, but it is always
   ** executed at least once, so this is more efficient.
   */
  j=1;
  do {
    coeffDeriv[j] = coeff[j-1] - coeff[j];
    j++;
  } while (j < order - 1);
  coeffDeriv[j] = coeff[j-1];
  
  oldval = coeff[0] * vprime;
  coeff[0] = oneMinusvprime * coeff[0];
  for (j = 1; j < i; j++) {
    temp = oldval;
    oldval = coeff[j] * vprime;
    coeff[j] = temp + oneMinusvprime * coeff[j];
  }
  coeff[j] = oldval;
}

void OpenGLSurfaceEvaluator::inEvalULine(int n_points, REAL v, REAL* u_vals, 
	int stride, REAL ret_points[][3], REAL ret_normals[][3])
{
  int i,k;
  REAL temp[4];
inPreEvaluateBV_intfac(v);

  for(i=0,k=0; i<n_points; i++, k += stride)
    {
      inDoEvalCoord2NOGE_BV(u_vals[k],v,temp, ret_normals[i]);

      ret_points[i][0] = temp[0];
      ret_points[i][1] = temp[1];
      ret_points[i][2] = temp[2];

    }

}

void OpenGLSurfaceEvaluator::inEvalVLine(int n_points, REAL u, REAL* v_vals, 
	int stride, REAL ret_points[][3], REAL ret_normals[][3])
{
  int i,k;
  REAL temp[4];
inPreEvaluateBU_intfac(u);
  for(i=0,k=0; i<n_points; i++, k += stride)
    {
      inDoEvalCoord2NOGE_BU(u, v_vals[k], temp, ret_normals[i]);
      ret_points[i][0] = temp[0];
      ret_points[i][1] = temp[1];
      ret_points[i][2] = temp[2];
    }
}
      

/*triangulate a strip bounded by two lines which are parallel  to U-axis
 *upperVerts: the verteces on the upper line
 *lowerVertx: the verteces on the lower line
 *n_upper >=1
 *n_lower >=1
 */
void OpenGLSurfaceEvaluator::inEvalUStrip(int n_upper, REAL v_upper, REAL* upper_val, int n_lower, REAL v_lower, REAL* lower_val)
{
  int i,j,k,l;
  REAL leftMostV[2];
 typedef REAL REAL3[3];

  REAL3* upperXYZ = (REAL3*) malloc(sizeof(REAL3)*n_upper);
  assert(upperXYZ);
  REAL3* upperNormal = (REAL3*) malloc(sizeof(REAL3) * n_upper);
  assert(upperNormal);
  REAL3* lowerXYZ = (REAL3*) malloc(sizeof(REAL3)*n_lower);
  assert(lowerXYZ);
  REAL3* lowerNormal = (REAL3*) malloc(sizeof(REAL3) * n_lower);
  assert(lowerNormal);
  
  inEvalULine(n_upper, v_upper, upper_val,  1, upperXYZ, upperNormal);
  inEvalULine(n_lower, v_lower, lower_val,  1, lowerXYZ, lowerNormal);



  REAL* leftMostXYZ;
  REAL* leftMostNormal;

  /*
   *the algorithm works by scanning from left to right.
   *leftMostV: the left most of the remaining verteces (on both upper and lower).
   *           it could an element of upperVerts or lowerVerts.
   *i: upperVerts[i] is the first vertex to the right of leftMostV on upper line   *j: lowerVerts[j] is the first vertex to the right of leftMostV on lower line   */

  /*initialize i,j,and leftMostV
   */
  if(upper_val[0] <= lower_val[0])
    {
      i=1;
      j=0;

      leftMostV[0] = upper_val[0];
      leftMostV[1] = v_upper;
      leftMostXYZ = upperXYZ[0];
      leftMostNormal = upperNormal[0];
    }
  else
    {
      i=0;
      j=1;

      leftMostV[0] = lower_val[0];
      leftMostV[1] = v_lower;

      leftMostXYZ = lowerXYZ[0];
      leftMostNormal = lowerNormal[0];
    }
  
  /*the main loop.
   *the invariance is that: 
   *at the beginning of each loop, the meaning of i,j,and leftMostV are 
   *maintained
   */
  while(1)
    {
      if(i >= n_upper) /*case1: no more in upper*/
        {
          if(j<n_lower-1) /*at least two vertices in lower*/
            {
              bgntfan();
	      glNormal3fv(leftMostNormal);
              glVertex3fv(leftMostXYZ);

              while(j<n_lower){
		glNormal3fv(lowerNormal[j]);
		glVertex3fv(lowerXYZ[j]);
		j++;

              }
              endtfan();
            }
          break; /*exit the main loop*/
        }
      else if(j>= n_lower) /*case2: no more in lower*/
        {
          if(i<n_upper-1) /*at least two vertices in upper*/
            {
              bgntfan();
	      glNormal3fv(leftMostNormal);
	      glVertex3fv(leftMostXYZ);
	      
              for(k=n_upper-1; k>=i; k--) /*reverse order for two-side lighting*/
		{
		  glNormal3fv(upperNormal[k]);
		  glVertex3fv(upperXYZ[k]);
		}

              endtfan();
            }
          break; /*exit the main loop*/
        }
      else /* case3: neither is empty, plus the leftMostV, there is at least one triangle to output*/
        {
          if(upper_val[i] <= lower_val[j])
            {
	      bgntfan();

	      glNormal3fv(lowerNormal[j]);
	      glVertex3fv(lowerXYZ[j]);

              /*find the last k>=i such that 
               *upperverts[k][0] <= lowerverts[j][0]
               */
              k=i;

              while(k<n_upper)
                {
                  if(upper_val[k] > lower_val[j])
                    break;
                  k++;

                }
              k--;


              for(l=k; l>=i; l--)/*the reverse is for two-side lighting*/
                {
		  glNormal3fv(upperNormal[l]);
		  glVertex3fv(upperXYZ[l]);

                }
	      glNormal3fv(leftMostNormal);
	      glVertex3fv(leftMostXYZ);

              endtfan();

              /*update i and leftMostV for next loop
               */
              i = k+1;

	      leftMostV[0] = upper_val[k];
	      leftMostV[1] = v_upper;
	      leftMostNormal = upperNormal[k];
	      leftMostXYZ = upperXYZ[k];
            }
          else /*upperVerts[i][0] > lowerVerts[j][0]*/
            {
	      bgntfan();
	      glNormal3fv(upperNormal[i]);
	      glVertex3fv(upperXYZ[i]);
	      
              glNormal3fv(leftMostNormal);
	      glVertex3fv(leftMostXYZ);
	      

              /*find the last k>=j such that
               *lowerverts[k][0] < upperverts[i][0]
               */
              k=j;
              while(k< n_lower)
                {
                  if(lower_val[k] >= upper_val[i])
                    break;
		  glNormal3fv(lowerNormal[k]);
		  glVertex3fv(lowerXYZ[k]);

                  k++;
                }
              endtfan();

              /*update j and leftMostV for next loop
               */
              j=k;
	      leftMostV[0] = lower_val[j-1];
	      leftMostV[1] = v_lower;

	      leftMostNormal = lowerNormal[j-1];
	      leftMostXYZ = lowerXYZ[j-1];
            }     
        }
    }
  //clean up 
  free(upperXYZ);
  free(lowerXYZ);
  free(upperNormal);
  free(lowerNormal);
}

/*triangulate a strip bounded by two lines which are parallel  to V-axis
 *leftVerts: the verteces on the left line
 *rightVertx: the verteces on the right line
 *n_left >=1
 *n_right >=1
 */
void OpenGLSurfaceEvaluator::inEvalVStrip(int n_left, REAL u_left, REAL* left_val, int n_right, REAL u_right, REAL* right_val)
{
  int i,j,k,l;
  REAL botMostV[2];
  typedef REAL REAL3[3];

  REAL3* leftXYZ = (REAL3*) malloc(sizeof(REAL3)*n_left);
  assert(leftXYZ);
  REAL3* leftNormal = (REAL3*) malloc(sizeof(REAL3) * n_left);
  assert(leftNormal);
  REAL3* rightXYZ = (REAL3*) malloc(sizeof(REAL3)*n_right);
  assert(rightXYZ);
  REAL3* rightNormal = (REAL3*) malloc(sizeof(REAL3) * n_right);
  assert(rightNormal);
  
  inEvalVLine(n_left, u_left, left_val,  1, leftXYZ, leftNormal);
  inEvalVLine(n_right, u_right, right_val,  1, rightXYZ, rightNormal);



  REAL* botMostXYZ;
  REAL* botMostNormal;

  /*
   *the algorithm works by scanning from bot to top.
   *botMostV: the bot most of the remaining verteces (on both left and right).
   *           it could an element of leftVerts or rightVerts.
   *i: leftVerts[i] is the first vertex to the top of botMostV on left line   
   *j: rightVerts[j] is the first vertex to the top of botMostV on rightline   */

  /*initialize i,j,and botMostV
   */
  if(left_val[0] <= right_val[0])
    {
      i=1;
      j=0;

      botMostV[0] = u_left;
      botMostV[1] = left_val[0];
      botMostXYZ = leftXYZ[0];
      botMostNormal = leftNormal[0];
    }
  else
    {
      i=0;
      j=1;

      botMostV[0] = u_right;
      botMostV[1] = right_val[0];

      botMostXYZ = rightXYZ[0];
      botMostNormal = rightNormal[0];
    }
  
  /*the main loop.
   *the invariance is that: 
   *at the beginning of each loop, the meaning of i,j,and botMostV are 
   *maintained
   */
  while(1)
    {
      if(i >= n_left) /*case1: no more in left*/
        {
          if(j<n_right-1) /*at least two vertices in right*/
            {
              bgntfan();
	      glNormal3fv(botMostNormal);
              glVertex3fv(botMostXYZ);

              while(j<n_right){
		glNormal3fv(rightNormal[j]);
		glVertex3fv(rightXYZ[j]);
		j++;

              }
              endtfan();
            }
          break; /*exit the main loop*/
        }
      else if(j>= n_right) /*case2: no more in right*/
        {
          if(i<n_left-1) /*at least two vertices in left*/
            {
              bgntfan();
	      glNormal3fv(botMostNormal);
	      glVertex3fv(botMostXYZ);
	      
              for(k=n_left-1; k>=i; k--) /*reverse order for two-side lighting*/
		{
		  glNormal3fv(leftNormal[k]);
		  glVertex3fv(leftXYZ[k]);
		}

              endtfan();
            }
          break; /*exit the main loop*/
        }
      else /* case3: neither is empty, plus the botMostV, there is at least one triangle to output*/
        {
          if(left_val[i] <= right_val[j])
            {
	      bgntfan();

	      glNormal3fv(rightNormal[j]);
	      glVertex3fv(rightXYZ[j]);

              /*find the last k>=i such that 
               *leftverts[k][0] <= rightverts[j][0]
               */
              k=i;

              while(k<n_left)
                {
                  if(left_val[k] > right_val[j])
                    break;
                  k++;

                }
              k--;


              for(l=k; l>=i; l--)/*the reverse is for two-side lighting*/
                {
		  glNormal3fv(leftNormal[l]);
		  glVertex3fv(leftXYZ[l]);

                }
	      glNormal3fv(botMostNormal);
	      glVertex3fv(botMostXYZ);

              endtfan();

              /*update i and botMostV for next loop
               */
              i = k+1;

	      botMostV[0] = u_left;
	      botMostV[1] = left_val[k];
	      botMostNormal = leftNormal[k];
	      botMostXYZ = leftXYZ[k];
            }
          else /*left_val[i] > right_val[j])*/
            {
	      bgntfan();
	      glNormal3fv(leftNormal[i]);
	      glVertex3fv(leftXYZ[i]);
	      
              glNormal3fv(botMostNormal);
	      glVertex3fv(botMostXYZ);
	      

              /*find the last k>=j such that
               *rightverts[k][0] < leftverts[i][0]
               */
              k=j;
              while(k< n_right)
                {
                  if(right_val[k] >= left_val[i])
                    break;
		  glNormal3fv(rightNormal[k]);
		  glVertex3fv(rightXYZ[k]);

                  k++;
                }
              endtfan();

              /*update j and botMostV for next loop
               */
              j=k;
	      botMostV[0] = u_right;
	      botMostV[1] = right_val[j-1];

	      botMostNormal = rightNormal[j-1];
	      botMostXYZ = rightXYZ[j-1];
            }     
        }
    }
  //clean up 
  free(leftXYZ);
  free(rightXYZ);
  free(leftNormal);
  free(rightNormal);
}

/*-----------------------begin evalMachine-------------------*/
void OpenGLSurfaceEvaluator::inMap2fEM(int which, int k,
	     REAL ulower,
	     REAL uupper,
	     int ustride,
	     int uorder,
	     REAL vlower,
	     REAL vupper,
	     int vstride,
	     int vorder,
	     REAL *ctlPoints)
{
  int i,j,x;
  surfEvalMachine *temp_em;
  switch(which){
  case 0: //vertex
    vertex_flag = 1;
    temp_em = &em_vertex;
    break;
  case 1: //normal
    normal_flag = 1;
    temp_em = &em_normal;
    break;
  case 2: //color
    color_flag = 1;
    temp_em = &em_color;
    break;
  default:
    texcoord_flag = 1;
    temp_em = &em_texcoord;
    break;
  }

  REAL *data = temp_em->ctlPoints;
  
  temp_em->uprime = -1;//initilized
  temp_em->vprime = -1;

  temp_em->k = k;
  temp_em->u1 = ulower;
  temp_em->u2 = uupper;
  temp_em->ustride = ustride;
  temp_em->uorder = uorder;
  temp_em->v1 = vlower;
  temp_em->v2 = vupper;
  temp_em->vstride = vstride;
  temp_em->vorder = vorder;

  /*copy the contrl points from ctlPoints to global_ev_ctlPoints*/
  for (i=0; i<uorder; i++) {
    for (j=0; j<vorder; j++) {
      for (x=0; x<k; x++) {
	data[x] = ctlPoints[x];
      }
      ctlPoints += vstride;
      data += k;
    }
    ctlPoints += ustride - vstride * vorder;
  }
}

void OpenGLSurfaceEvaluator::inDoDomain2WithDerivsEM(surfEvalMachine *em, REAL u, REAL v, 
				REAL *retPoint, REAL *retdu, REAL *retdv)
{
    int j, row, col;
    REAL the_uprime;
    REAL the_vprime;
    REAL p;
    REAL pdv;
    REAL *data;

    if((em->u2 == em->u1) || (em->v2 == em->v1))
	return;
    the_uprime = (u - em->u1) / (em->u2 - em->u1);
    the_vprime = (v - em->v1) / (em->v2 - em->v1);
    
    /* Compute coefficients for values and derivs */

    /* Use already cached values if possible */
    if(em->uprime != the_uprime) {
        inPreEvaluateWithDeriv(em->uorder, the_uprime, em->ucoeff, em->ucoeffDeriv);
	em->uprime = the_uprime;
    }
    if (em->vprime != the_vprime) {
	inPreEvaluateWithDeriv(em->vorder, the_vprime, em->vcoeff, em->vcoeffDeriv);
	em->vprime = the_vprime;
    }

    for (j = 0; j < em->k; j++) {
	data=em->ctlPoints+j;
	retPoint[j] = retdu[j] = retdv[j] = 0.0;
	for (row = 0; row < em->uorder; row++)  {
	    /* 
	    ** Minor optimization.
	    ** The col == 0 part of the loop is extracted so we don't
	    ** have to initialize p and pdv to 0.
	    */
	    p = em->vcoeff[0] * (*data);
	    pdv = em->vcoeffDeriv[0] * (*data);
	    data += em->k;
	    for (col = 1; col < em->vorder; col++) {
		/* Incrementally build up p, pdv value */
		p += em->vcoeff[col] * (*data);
		pdv += em->vcoeffDeriv[col] * (*data);
		data += em->k;
	    }
	    /* Use p, pdv value to incrementally add up r, du, dv */
	    retPoint[j] += em->ucoeff[row] * p;
	    retdu[j] += em->ucoeffDeriv[row] * p;
	    retdv[j] += em->ucoeff[row] * pdv;
	}
    }  
}  

void OpenGLSurfaceEvaluator::inDoDomain2EM(surfEvalMachine *em, REAL u, REAL v, 
				REAL *retPoint)
{
    int j, row, col;
    REAL the_uprime;
    REAL the_vprime;
    REAL p;
    REAL *data;

    if((em->u2 == em->u1) || (em->v2 == em->v1))
	return;
    the_uprime = (u - em->u1) / (em->u2 - em->u1);
    the_vprime = (v - em->v1) / (em->v2 - em->v1);
    
    /* Compute coefficients for values and derivs */

    /* Use already cached values if possible */
    if(em->uprime != the_uprime) {
        inPreEvaluate(em->uorder, the_uprime, em->ucoeff);
	em->uprime = the_uprime;
    }
    if (em->vprime != the_vprime) {
	inPreEvaluate(em->vorder, the_vprime, em->vcoeff);
	em->vprime = the_vprime;
    }

    for (j = 0; j < em->k; j++) {
	data=em->ctlPoints+j;
	retPoint[j] = 0.0;
	for (row = 0; row < em->uorder; row++)  {
	    /* 
	    ** Minor optimization.
	    ** The col == 0 part of the loop is extracted so we don't
	    ** have to initialize p and pdv to 0.
	    */
	    p = em->vcoeff[0] * (*data);
	    data += em->k;
	    for (col = 1; col < em->vorder; col++) {
		/* Incrementally build up p, pdv value */
		p += em->vcoeff[col] * (*data);
		data += em->k;
	    }
	    /* Use p, pdv value to incrementally add up r, du, dv */
	    retPoint[j] += em->ucoeff[row] * p;
	}
    }  
}  


void OpenGLSurfaceEvaluator::inDoEvalCoord2EM(REAL u, REAL v)
{
  REAL temp_vertex[5];
  REAL temp_normal[3];
  REAL temp_color[4];
  REAL temp_texcoord[4];

  if(texcoord_flag)
    {
      inDoDomain2EM(&em_texcoord, u,v, temp_texcoord);
      texcoordCallBack(temp_texcoord, userData);
    }
  if(color_flag)
    {
      inDoDomain2EM(&em_color, u,v, temp_color);
      colorCallBack(temp_color, userData);
    }

  if(normal_flag) //there is a normla map
    {
      inDoDomain2EM(&em_normal, u,v, temp_normal);
      normalCallBack(temp_normal, userData);
    
      if(vertex_flag)
	{
	  inDoDomain2EM(&em_vertex, u,v,temp_vertex);
	  if(em_vertex.k == 4)
	    {
	      temp_vertex[0] /= temp_vertex[3];
	      temp_vertex[1] /= temp_vertex[3];
	      temp_vertex[2] /= temp_vertex[3];	      
	    }
          temp_vertex[3]=u;
          temp_vertex[4]=v;	  
	  vertexCallBack(temp_vertex, userData);
	}
    }
  else if(auto_normal_flag) //no normal map but there is a normal callbackfunctin
    {
      REAL du[4];
      REAL dv[4];
      
      /*compute homegeneous point and partial derivatives*/
      inDoDomain2WithDerivsEM(&em_vertex, u,v,temp_vertex,du,dv);

      if(em_vertex.k ==4)
	inComputeFirstPartials(temp_vertex, du, dv);

#ifdef AVOID_ZERO_NORMAL
      if(myabs(dv[0]) <= MYZERO && myabs(dv[1]) <= MYZERO && myabs(dv[2]) <= MYZERO)
	{
	  
	  REAL tempdu[4];
	  REAL tempdata[4];
	  REAL u1 = em_vertex.u1;
	  REAL u2 = em_vertex.u2;
	  if(u-MYDELTA*(u2-u1) < u1)
	    u = u+ MYDELTA*(u2-u1);
	  else
	    u = u-MYDELTA*(u2-u1);
	  inDoDomain2WithDerivsEM(&em_vertex,u,v, tempdata, tempdu, dv);

	  if(em_vertex.k ==4)
	    inComputeFirstPartials(temp_vertex, du, dv);	  
	}
      else if(myabs(du[0]) <= MYZERO && myabs(du[1]) <= MYZERO && myabs(du[2]) <= MYZERO)
	{
	  REAL tempdv[4];
	  REAL tempdata[4];
	  REAL v1 = em_vertex.v1;
	  REAL v2 = em_vertex.v2;
	  if(v-MYDELTA*(v2-v1) < v1)
	    v = v+ MYDELTA*(v2-v1);
	  else
	    v = v-MYDELTA*(v2-v1);
	  inDoDomain2WithDerivsEM(&em_vertex,u,v, tempdata, du, tempdv);

	  if(em_vertex.k ==4)
	    inComputeFirstPartials(temp_vertex, du, dv);
	}
#endif

      /*compute normal*/
      switch(em_vertex.k){
      case 3:

	inComputeNormal2(du, dv, temp_normal);
	break;
      case 4:

//	inComputeFirstPartials(temp_vertex, du, dv);
	inComputeNormal2(du, dv, temp_normal);

	/*transform the homegeneous coordinate of retPoint into inhomogenous one*/
	temp_vertex[0] /= temp_vertex[3];
	temp_vertex[1] /= temp_vertex[3];
	temp_vertex[2] /= temp_vertex[3];
	break;
      }
      normalCallBack(temp_normal, userData);
      temp_vertex[3] = u;
      temp_vertex[4] = v;
      vertexCallBack(temp_vertex, userData);
      
    }/*end if auto_normal*/
  else //no normal map, and no normal callback function
    {
      if(vertex_flag)
	{
	  inDoDomain2EM(&em_vertex, u,v,temp_vertex);
	  if(em_vertex.k == 4)
	    {
	      temp_vertex[0] /= temp_vertex[3];
	      temp_vertex[1] /= temp_vertex[3];
	      temp_vertex[2] /= temp_vertex[3];	      
	    }
          temp_vertex[3] = u;
          temp_vertex[4] = v;
	  vertexCallBack(temp_vertex, userData);
	}
    }
}


void OpenGLSurfaceEvaluator::inBPMEvalEM(bezierPatchMesh* bpm)
{
  int i,j,k;
  float u,v;

  int ustride;
  int vstride;

#ifdef USE_LOD
  if(bpm->bpatch != NULL)
    {
      bezierPatch* p=bpm->bpatch;
      ustride = p->dimension * p->vorder;
      vstride = p->dimension;

      glMap2f( (p->dimension == 3)? GL_MAP2_VERTEX_3 : GL_MAP2_VERTEX_4,
	      p->umin,
	      p->umax,
	      ustride,
	      p->uorder,
	      p->vmin,
	      p->vmax,
	      vstride,
	      p->vorder,
	      p->ctlpoints);


/*
    inMap2fEM(0, p->dimension,
	  p->umin,
	  p->umax,
	  ustride,
	  p->uorder,
	  p->vmin,
	  p->vmax,
	  vstride,
	  p->vorder,
	  p->ctlpoints);
*/
    }
#else

  if(bpm->bpatch != NULL){
    bezierPatch* p = bpm->bpatch;
    ustride = p->dimension * p->vorder;
    vstride = p->dimension;
    inMap2fEM(0, p->dimension,
	  p->umin,
	  p->umax,
	  ustride,
	  p->uorder,
	  p->vmin,
	  p->vmax,
	  vstride,
	  p->vorder,
	  p->ctlpoints);
  }
  if(bpm->bpatch_normal != NULL){
    bezierPatch* p = bpm->bpatch_normal;
    ustride = p->dimension * p->vorder;
    vstride = p->dimension;
    inMap2fEM(1, p->dimension,
	  p->umin,
	  p->umax,
	  ustride,
	  p->uorder,
	  p->vmin,
	  p->vmax,
	  vstride,
	  p->vorder,
	  p->ctlpoints);
  }
  if(bpm->bpatch_color != NULL){
    bezierPatch* p = bpm->bpatch_color;
    ustride = p->dimension * p->vorder;
    vstride = p->dimension;
    inMap2fEM(2, p->dimension,
	  p->umin,
	  p->umax,
	  ustride,
	  p->uorder,
	  p->vmin,
	  p->vmax,
	  vstride,
	  p->vorder,
	  p->ctlpoints);
  }
  if(bpm->bpatch_texcoord != NULL){
    bezierPatch* p = bpm->bpatch_texcoord;
    ustride = p->dimension * p->vorder;
    vstride = p->dimension;
    inMap2fEM(3, p->dimension,
	  p->umin,
	  p->umax,
	  ustride,
	  p->uorder,
	  p->vmin,
	  p->vmax,
	  vstride,
	  p->vorder,
	  p->ctlpoints);
  }
#endif


  k=0;
  for(i=0; i<bpm->index_length_array; i++)
    {
#ifdef USE_LOD
      if(bpm->type_array[i] == GL_POLYGON) //a mesh
	{
	  GLfloat *temp = bpm->UVarray+k;
	  GLfloat u0 = temp[0];
	  GLfloat v0 = temp[1];
	  GLfloat u1 = temp[2];
	  GLfloat v1 = temp[3];
	  GLint nu = (GLint) ( temp[4]);
	  GLint nv = (GLint) ( temp[5]);
	  GLint umin = (GLint) ( temp[6]);
	  GLint vmin = (GLint) ( temp[7]);
	  GLint umax = (GLint) ( temp[8]);
	  GLint vmax = (GLint) ( temp[9]);

	  glMapGrid2f(LOD_eval_level*nu, u0, u1, LOD_eval_level*nv, v0, v1);
	  glEvalMesh2(GL_FILL, LOD_eval_level*umin, LOD_eval_level*umax, LOD_eval_level*vmin, LOD_eval_level*vmax);
	}
      else
	{
	  LOD_eval(bpm->length_array[i], bpm->UVarray+k, bpm->type_array[i],
		   0
		   );
	}
	  k+= 2*bpm->length_array[i];       
    
#else //undef  USE_LOD

#ifdef CRACK_TEST
if(  bpm->bpatch->umin == 2 &&   bpm->bpatch->umax == 3
  && bpm->bpatch->vmin ==2 &&    bpm->bpatch->vmax == 3)
{
REAL vertex[4];
REAL normal[4];
#ifdef DEBUG
printf("***number ****1\n");
#endif

beginCallBack(GL_QUAD_STRIP, NULL);
inDoEvalCoord2EM(3.0, 3.0);
inDoEvalCoord2EM(2.0, 3.0);
inDoEvalCoord2EM(3.0, 2.7);
inDoEvalCoord2EM(2.0, 2.7);
inDoEvalCoord2EM(3.0, 2.0);
inDoEvalCoord2EM(2.0, 2.0);
endCallBack(NULL);

beginCallBack(GL_TRIANGLE_STRIP, NULL);
inDoEvalCoord2EM(2.0, 3.0);
inDoEvalCoord2EM(2.0, 2.0);
inDoEvalCoord2EM(2.0, 2.7);
endCallBack(NULL);

}
if(  bpm->bpatch->umin == 1 &&   bpm->bpatch->umax == 2
  && bpm->bpatch->vmin ==2 &&    bpm->bpatch->vmax == 3)
{
#ifdef DEBUG
printf("***number 3\n");
#endif
beginCallBack(GL_QUAD_STRIP, NULL);
inDoEvalCoord2EM(2.0, 3.0);
inDoEvalCoord2EM(1.0, 3.0);
inDoEvalCoord2EM(2.0, 2.3);
inDoEvalCoord2EM(1.0, 2.3);
inDoEvalCoord2EM(2.0, 2.0);
inDoEvalCoord2EM(1.0, 2.0);
endCallBack(NULL);

beginCallBack(GL_TRIANGLE_STRIP, NULL);
inDoEvalCoord2EM(2.0, 2.3);
inDoEvalCoord2EM(2.0, 2.0);
inDoEvalCoord2EM(2.0, 3.0);
endCallBack(NULL);

}
return;
#endif //CRACK_TEST

      beginCallBack(bpm->type_array[i], userData);

      for(j=0; j<bpm->length_array[i]; j++)
	{
	  u = bpm->UVarray[k];
	  v = bpm->UVarray[k+1];
#ifdef USE_LOD
          LOD_EVAL_COORD(u,v);
//	  glEvalCoord2f(u,v);
#else

#ifdef  GENERIC_TEST
          float temp_normal[3];
          float temp_vertex[3];
          if(temp_signal == 0)
	    {
	      gTessVertexSphere(u,v, temp_normal, temp_vertex);
//printf("normal=(%f,%f,%f)\n", temp_normal[0], temp_normal[1], temp_normal[2])//printf("veretx=(%f,%f,%f)\n", temp_vertex[0], temp_vertex[1], temp_vertex[2]);
              normalCallBack(temp_normal, userData);
	      vertexCallBack(temp_vertex, userData);
	    }
          else if(temp_signal == 1)
	    {
	      gTessVertexCyl(u,v, temp_normal, temp_vertex);
//printf("normal=(%f,%f,%f)\n", temp_normal[0], temp_normal[1], temp_normal[2])//printf("veretx=(%f,%f,%f)\n", temp_vertex[0], temp_vertex[1], temp_vertex[2]);
              normalCallBack(temp_normal, userData);
	      vertexCallBack(temp_vertex, userData);
	    }
	  else
#endif //GENERIC_TEST

	    inDoEvalCoord2EM(u,v);
     
#endif //USE_LOD

	  k += 2;
	}
      endCallBack(userData);

#endif //USE_LOD
    }
}

void OpenGLSurfaceEvaluator::inBPMListEvalEM(bezierPatchMesh* list)
{
  bezierPatchMesh* temp;
  for(temp = list; temp != NULL; temp = temp->next)
    {
      inBPMEvalEM(temp);
    }
}

