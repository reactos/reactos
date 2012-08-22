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
#include <assert.h>
#include <math.h>
#include "bezierEval.h"

#ifdef __WATCOMC__
#pragma warning 14  10
#endif

#define TOLERANCE 0.0001

#ifndef MAX_ORDER
#define MAX_ORDER 16
#endif

#ifndef MAX_DIMENSION
#define MAX_DIMENSION 4
#endif

static void normalize(float vec[3]);
static void crossProduct(float x[3], float y[3], float ret[3]);
#if 0 // UNUSED
static void bezierCurveEvalfast(float u0, float u1, int order, float *ctlpoints, int stride,  int dimension, float u, float retpoint[]);
#endif

static float binomialCoefficients[8][8] = {
  {1,0,0,0,0,0,0,0},
  {1,1,0,0,0,0,0,0},
  {1,2,1,0,0,0,0,0},
  {1,3,3,1,0,0,0,0},
  {1,4,6,4,1,0,0,0},
  {1,5,10,10,5,1,0,0},
  {1,6,15,20,15,6,1,0},
  {1,7,21,35,35,21,7,1}
};

void bezierCurveEval(float u0, float u1, int order, float *ctlpoints, int stride, int dimension, float u, float retpoint[])
{
  float uprime = (u-u0)/(u1-u0);
  float *ctlptr = ctlpoints;
  float oneMinusX = 1.0f-uprime;
  float XPower = 1.0f;

  int i,k;
  for(k=0; k<dimension; k++)
    retpoint[k] = (*(ctlptr + k));

  for(i=1; i<order; i++){
    ctlptr += stride;
    XPower *= uprime;
    for(k=0; k<dimension; k++) {
      retpoint[k] = retpoint[k]*oneMinusX + ctlptr[k]* binomialCoefficients[order-1][i] * XPower;
    }
  }
}


#if 0 // UNUSED
/*order = degree +1 >=1.
 */
void bezierCurveEvalfast(float u0, float u1, int order, float *ctlpoints, int stride,  int dimension, float u, float retpoint[])
{
  float uprime = (u-u0)/(u1-u0);
  float buf[MAX_ORDER][MAX_ORDER][MAX_DIMENSION];
  float* ctlptr = ctlpoints;
  int r, i,j;
  for(i=0; i<order; i++) {
    for(j=0; j<dimension; j++)
      buf[0][i][j] = ctlptr[j];
    ctlptr += stride;
  }
  for(r=1; r<order; r++){
    for(i=0; i<order-r; i++) {
      for(j=0; j<dimension; j++)
	buf[r][i][j] = (1-uprime)*buf[r-1][i][j] + uprime*buf[r-1][i+1][j];
    }
  }

  for(j=0; j<dimension; j++)
    retpoint[j] = buf[order-1][0][j];
}
#endif


/*order = degree +1 >=1.
 */
void bezierCurveEvalDer(float u0, float u1, int order, float *ctlpoints, int stride,  int dimension, float u, float retDer[])
{
  int i,k;
  float width = u1-u0;
  float *ctlptr = ctlpoints;

  float buf[MAX_ORDER][MAX_DIMENSION];
  if(order == 1){
    for(k=0; k<dimension; k++)
      retDer[k]=0;
  }
  for(i=0; i<order-1; i++){
    for(k=0; k<dimension; k++) {
      buf[i][k] = (ctlptr[stride+k] - ctlptr[k])*(order-1)/width;
    }
    ctlptr += stride;
  }

  bezierCurveEval(u0, u1, order-1, (float*) buf, MAX_DIMENSION,  dimension, u, retDer);
}

void bezierCurveEvalDerGen(int der, float u0, float u1, int order, float *ctlpoints, int stride,  int dimension, float u, float retDer[])
{
  int i,k,r;
  float *ctlptr = ctlpoints;
  float width=u1-u0;
  float buf[MAX_ORDER][MAX_ORDER][MAX_DIMENSION];
  if(der<0) der=0;
  for(i=0; i<order; i++){
    for(k=0; k<dimension; k++){
      buf[0][i][k] = ctlptr[k];
    }
    ctlptr += stride;
  }


  for(r=1; r<=der; r++){
    for(i=0; i<order-r; i++){
      for(k=0; k<dimension; k++){
	buf[r][i][k] = (buf[r-1][i+1][k] - buf[r-1][i][k])*(order-r)/width;
      }
    }
  }

  bezierCurveEval(u0, u1, order-der, (float *) (buf[der]), MAX_DIMENSION, dimension, u, retDer);
}

/*the Bezier bivarite polynomial is:
 * sum[i:0,uorder-1][j:0,vorder-1] { ctlpoints[i*ustride+j*vstride] * B(i)*B(j)
 * where B(i) and B(j) are basis functions
 */
void bezierSurfEvalDerGen(int uder, int vder, float u0, float u1, int uorder, float v0, float v1, int vorder, int dimension, float *ctlpoints, int ustride, int vstride, float u, float v, float ret[])
{
  int i;
  float newPoints[MAX_ORDER][MAX_DIMENSION];

  for(i=0; i<uorder; i++){

    bezierCurveEvalDerGen(vder, v0, v1, vorder, ctlpoints+ustride*i, vstride, dimension, v, newPoints[i]);

  }

  bezierCurveEvalDerGen(uder, u0, u1, uorder, (float *) newPoints, MAX_DIMENSION, dimension, u, ret);
}


/*division by w is performed*/
void bezierSurfEval(float u0, float u1, int uorder, float v0, float v1, int vorder, int dimension, float *ctlpoints, int ustride, int vstride, float u, float v, float ret[])
{
  bezierSurfEvalDerGen(0, 0, u0, u1, uorder, v0, v1, vorder, dimension, ctlpoints, ustride, vstride, u, v, ret);
  if(dimension == 4) /*homogeneous*/{
    ret[0] /= ret[3];
    ret[1] /= ret[3];
    ret[2] /= ret[3];
  }
}

void bezierSurfEvalNormal(float u0, float u1, int uorder, float v0, float v1, int vorder, int dimension, float *ctlpoints, int ustride, int vstride, float u, float v, float retNormal[])
{
  float partialU[4];
  float partialV[4];
  assert(dimension>=3 && dimension <=4);
  bezierSurfEvalDerGen(1,0, u0, u1, uorder, v0, v1, vorder, dimension, ctlpoints, ustride, vstride, u, v, partialU);
  bezierSurfEvalDerGen(0,1, u0, u1, uorder, v0, v1, vorder, dimension, ctlpoints, ustride, vstride, u, v, partialV);

  if(dimension == 3){/*inhomogeneous*/
    crossProduct(partialU, partialV, retNormal);

    normalize(retNormal);

    return;
  }
  else { /*homogeneous*/
    float val[4]; /*the point coordinates (without derivative)*/
    float newPartialU[MAX_DIMENSION];
    float newPartialV[MAX_DIMENSION];
    int i;
    bezierSurfEvalDerGen(0,0, u0, u1, uorder, v0, v1, vorder, dimension, ctlpoints, ustride, vstride, u, v, val);

    for(i=0; i<=2; i++){
      newPartialU[i] = partialU[i] * val[3] - val[i] * partialU[3];
      newPartialV[i] = partialV[i] * val[3] - val[i] * partialV[3];
    }
    crossProduct(newPartialU, newPartialV, retNormal);
    normalize(retNormal);
  }
}

/*if size is 0, then nothing is done*/
static void normalize(float vec[3])
{
  float size = (float)sqrt(vec[0]*vec[0] + vec[1]*vec[1] + vec[2]*vec[2]);

  if(size < TOLERANCE)
    {
#ifdef DEBUG
      fprintf(stderr, "Warning: in oglBSpline.c normal is 0\n");
#endif
      return;
    }
  else {
    vec[0] = vec[0]/size;
    vec[1] = vec[1]/size;
    vec[2] = vec[2]/size;
  }
}


static void crossProduct(float x[3], float y[3], float ret[3])
{
  ret[0] = x[1]*y[2] - y[1]*x[2];
  ret[1] = x[2]*y[0] - y[2]*x[0];
  ret[2] = x[0]*y[1] - y[0]*x[1];

}

