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

#include <stdlib.h>
#include <stdio.h>

#include "glcurveval.h"


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
void OpenGLCurveEvaluator::inPreEvaluate(int order, REAL vprime, REAL *coeff)
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

void OpenGLCurveEvaluator::inMap1f(int which, //0: vert, 1: norm, 2: color, 3: tex
				   int k, //dimension
				   REAL ulower,
				   REAL uupper,
				   int ustride,
				   int uorder,
				   REAL *ctlpoints)
{
  int i,x;
  curveEvalMachine *temp_em;
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
  
  REAL *data = temp_em->ctlpoints;
  temp_em->uprime = -1; //initialized
  temp_em->k = k;
  temp_em->u1 = ulower;
  temp_em->u2 = uupper;
  temp_em->ustride = ustride;
  temp_em->uorder = uorder;
  /*copy the control points*/
  for(i=0; i<uorder; i++){
    for(x=0; x<k; x++){
      data[x] = ctlpoints[x];
    }
    ctlpoints += ustride;
    data += k;
  }     
}

void OpenGLCurveEvaluator::inDoDomain1(curveEvalMachine *em, REAL u, REAL *retPoint)
{
  int j, row;
  REAL the_uprime;
  REAL *data;
  
  if(em->u2 == em->u1)
    return;
  the_uprime = (u-em->u1) / (em->u2-em->u1);
  /*use already cached values if possible*/
  if(em->uprime != the_uprime){
    inPreEvaluate(em->uorder, the_uprime, em->ucoeff);
    em->uprime = the_uprime;
  }
  
  for(j=0; j<em->k; j++){
    data = em->ctlpoints+j;
    retPoint[j] = 0.0;
    for(row=0; row<em->uorder; row++)
      {
	retPoint[j] += em->ucoeff[row] * (*data);
	data += em->k;
      }
  } 
}

void  OpenGLCurveEvaluator::inDoEvalCoord1(REAL u)
{
  REAL temp_vertex[4];
  REAL temp_normal[3];
  REAL temp_color[4];
  REAL temp_texcoord[4];
  if(texcoord_flag) //there is a texture map
    {
      inDoDomain1(&em_texcoord, u, temp_texcoord);
      texcoordCallBack(temp_texcoord, userData);
    }
#ifdef DEBUG
printf("color_flag = %i\n", color_flag);
#endif
  if(color_flag) //there is a color map
    {
      inDoDomain1(&em_color, u, temp_color);
      colorCallBack(temp_color, userData);
    }
  if(normal_flag) //there is a normal map
    {
      inDoDomain1(&em_normal, u, temp_normal);
      normalCallBack(temp_normal, userData);
    }
  if(vertex_flag)
    {
      inDoDomain1(&em_vertex, u, temp_vertex);
      vertexCallBack(temp_vertex, userData);
    }
}

void OpenGLCurveEvaluator::inMapMesh1f(int umin, int umax)
{
  REAL du, u;
  int i;
  if(global_grid_nu == 0)
    return; //no points to output
  du = (global_grid_u1 - global_grid_u0) / (REAL) global_grid_nu;
  bgnline();
  for(i=umin; i<= umax; i++){
    u = (i==global_grid_nu)? global_grid_u1: global_grid_u0 + i*du;
    inDoEvalCoord1(u);
  }
  endline();
}
