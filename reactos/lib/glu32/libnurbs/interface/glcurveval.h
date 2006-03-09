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
*/

/*
 * glcurveval.h
 *
 */

#ifndef __gluglcurveval_h_
#define __gluglcurveval_h_

#include "gluos.h"
#include <GL/gl.h>
#include <GL/glu.h>
#include "basiccrveval.h"

class CurveMap;

/*for internal evaluator callback stuff*/
#ifndef IN_MAX_BEZIER_ORDER
#define IN_MAX_BEZIER_ORDER 40 /*XXX should be bigger than machine order*/
#endif
			
#ifndef IN_MAX_DIMENSION
#define IN_MAX_DIMENSION 4 
#endif

typedef struct curveEvalMachine{
  REAL uprime; //cached previously evaluated uprime
  int k; //the dimension
  REAL u1;
  REAL u2;
  int ustride;
  int uorder;
  REAL ctlpoints[IN_MAX_BEZIER_ORDER*IN_MAX_DIMENSION];
  REAL ucoeff[IN_MAX_BEZIER_ORDER];//cache the polynomial values
} curveEvalMachine;

class OpenGLCurveEvaluator : public BasicCurveEvaluator  {  
public:
			OpenGLCurveEvaluator(void);
			virtual ~OpenGLCurveEvaluator(void);
    void		range1f(long, REAL *, REAL *);
    void		domain1f(REAL, REAL);
    void		addMap(CurveMap *);

    void		enable(long);
    void		disable(long);
    void		bgnmap1f(long);
    void		map1f(long, REAL, REAL, long, long, REAL *);
    void		mapgrid1f(long, REAL, REAL);
    void		mapmesh1f(long, long, long);
    void		evalpoint1i(long);
    void		evalcoord1f(long, REAL);
    void		endmap1f(void);

    void		bgnline(void);
    void		endline(void);

    void                put_vertices_call_back(int flag)
      {
	output_triangles = flag;
      }
#ifdef _WIN32
    void               putCallBack(GLenum which, void (APIENTRY *fn)() );
#else
    void               putCallBack(GLenum which, _GLUfuncptr fn );
#endif
    void               set_callback_userData(void *data)
      {
	userData = data;
      }

/*------------------begin for curveEvalMachine------------*/
curveEvalMachine em_vertex;
curveEvalMachine em_normal;
curveEvalMachine em_color;
curveEvalMachine em_texcoord;
int vertex_flag; //whether there is a vertex map or not
int normal_flag; //whether there is a normal map or not
int color_flag; //whether there is a color map or not
int texcoord_flag; //whether there is a texture map or not

REAL global_grid_u0;
REAL global_grid_u1;
int global_grid_nu;

void inMap1f(int which, //0: vert, 1: norm, 2: color, 3: tex
	     int dimension,
	     REAL ulower,
	     REAL uupper,
	     int ustride,
	     int uorder,
	     REAL *ctlpoints);

void inPreEvaluate(int order, REAL vprime, REAL *coeff);
void inDoDomain1(curveEvalMachine *em, REAL u, REAL *retPoint);
void inDoEvalCoord1(REAL u);
void inMapMesh1f(int umin, int umax);

void     (GLAPIENTRY *beginCallBackN) (GLenum type);
void     (GLAPIENTRY *endCallBackN)   (void);
void     (GLAPIENTRY *vertexCallBackN) (const GLfloat *vert);
void     (GLAPIENTRY *normalCallBackN) (const GLfloat *normal);
void     (GLAPIENTRY *colorCallBackN) (const GLfloat *color);
void     (GLAPIENTRY *texcoordCallBackN) (const GLfloat *texcoord);

void     (GLAPIENTRY *beginCallBackData) (GLenum type, void* data);
void     (GLAPIENTRY *endCallBackData)   (void* data);
void     (GLAPIENTRY *vertexCallBackData) (const GLfloat *vert, void* data);
void     (GLAPIENTRY *normalCallBackData) (const GLfloat *normal, void* data);
void     (GLAPIENTRY *colorCallBackData) (const GLfloat *color, void* data);
void     (GLAPIENTRY *texcoordCallBackData) (const GLfloat *texcoord, void* data);

void* userData; //the opaque pointer for Data callback functions
void  beginCallBack(GLenum type, void* data);
void endCallBack(void* data);
void vertexCallBack(const GLfloat *vert, void *data);
void normalCallBack(const GLfloat *normal, void* data);
void colorCallBack(const  GLfloat *color, void* data);
void texcoordCallBack(const GLfloat *texcoord, void* data);


/*------------------end   for curveEvalMachine------------*/

private:
    int output_triangles; //true 1; false 0
};

#endif /* __gluglcurveval_h_ */
