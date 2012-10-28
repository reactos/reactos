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
#include <GL/gl.h>
#include <GL/glu.h>
#include <stdio.h>
#include "glimports.h"
#include "glrenderer.h"
#include "nurbsconsts.h"

//#define DOWN_LOAD_NURBS
#ifdef DOWN_LOAD_NURBS

#include "oglTrimNurbs.h"
static int surfcount = 0;
static oglTrimNurbs* otn = NULL;
nurbSurf* tempNurb = NULL;
oglTrimLoops* tempTrim = NULL;
#endif


//for LOD
extern "C" {void glu_LOD_eval_list(GLUnurbs *nurb, int level);}

void glu_LOD_eval_list(GLUnurbs *nurb, int level)
{
	nurb->LOD_eval_list(level);
}

GLUnurbs * GLAPIENTRY
gluNewNurbsRenderer(void)
{
  GLUnurbs *t;
  
  t = new GLUnurbs();
  return t;
}

void GLAPIENTRY
gluDeleteNurbsRenderer(GLUnurbs *r)
{
    delete r;
}

extern "C"
void GLAPIENTRY

gluDeleteNurbsTessellatorEXT(GLUnurbsObj *r)
{
  delete r;
}

void GLAPIENTRY
gluBeginSurface(GLUnurbs *r)
{
#ifdef DOWN_LOAD_NURBS
surfcount++;
tempTrim = OTL_make(10,10);
#endif
    r->bgnsurface(0); 
}

void GLAPIENTRY
gluBeginCurve(GLUnurbs *r)
{
    r->bgncurve(0); 
}

void GLAPIENTRY
gluEndCurve(GLUnurbs *r)
{
    r->endcurve(); 
}

void GLAPIENTRY
gluEndSurface(GLUnurbs *r)
{
#ifdef DOWN_LOAD_NURBS
if(surfcount == 1)
  otn = OTN_make(1);
OTN_insert(otn, tempNurb, tempTrim);
if(surfcount  >= 1)
{
#ifdef DEBUG
printf("write file\n");
#endif
OTN_write(otn, "out.otn");

}
#endif

    r->endsurface(); 
}

void GLAPIENTRY
gluBeginTrim(GLUnurbs *r)
{
#ifdef DOWN_LOAD_NURBS
OTL_bgnTrim(tempTrim);
#endif

    r->bgntrim(); 
}

void GLAPIENTRY
gluEndTrim(GLUnurbs *r)
{
#ifdef DOWN_LOAD_NURBS
OTL_endTrim(tempTrim);
#endif
    r->endtrim(); 
}

void GLAPIENTRY
gluPwlCurve(GLUnurbs *r, GLint count, INREAL array[], 
		GLint stride, GLenum type)
{
#ifdef DOWN_LOAD_NURBS
OTL_pwlCurve(tempTrim, count, array, stride, type);
#endif

    int realType;
    switch(type) {
      case GLU_MAP1_TRIM_2:
	realType = N_P2D;
	break;
      case GLU_MAP1_TRIM_3:
	realType = N_P2DR;
	break;
      default:
	realType = type;
	break;
    }
    r->pwlcurve(count, array, sizeof(INREAL) * stride, realType);
}

void GLAPIENTRY
gluNurbsCurve(GLUnurbs *r, GLint nknots, INREAL knot[], GLint stride, 
		  INREAL ctlarray[], GLint order, GLenum type)
{
#ifdef DOWN_LOAD_NURBS
OTL_nurbsCurve(tempTrim, nknots, knot, stride, ctlarray, order, type);
#endif

    int realType;

    switch(type) {
      case GLU_MAP1_TRIM_2:
	realType = N_P2D;
	break;
      case GLU_MAP1_TRIM_3:
	realType = N_P2DR;
	break;
      default:
	realType = type;
	break;
    }

    r->nurbscurve(nknots, knot, sizeof(INREAL) * stride, ctlarray, order, 
	    realType);
}

void GLAPIENTRY
gluNurbsSurface(GLUnurbs *r, GLint sknot_count, GLfloat *sknot, 
			    GLint tknot_count, GLfloat *tknot, 
			    GLint s_stride, GLint t_stride, 
			    GLfloat *ctlarray, GLint sorder, GLint torder, 
			    GLenum type)
{
#ifdef DOWN_LOAD_NURBS
  {
    int dimension;
    switch(type){
    case GL_MAP2_VERTEX_3:
      dimension = 3;
      break;
    case GL_MAP2_VERTEX_4:
      dimension = 4;
      break;
    default:
      fprintf(stderr, "error in glinterface.c++, type no implemented\n");
      exit(1);
    }
tempNurb = nurbSurfMake(sknot_count, sknot,
			tknot_count, tknot,
			sorder, torder,
			dimension,
			ctlarray,
			s_stride, t_stride);
			
  }
#endif

    r->nurbssurface(sknot_count, sknot, tknot_count, tknot, 
	    sizeof(INREAL) * s_stride, sizeof(INREAL) * t_stride, 
	    ctlarray, sorder, torder, type);
}

void GLAPIENTRY
gluLoadSamplingMatrices(GLUnurbs *r, const GLfloat modelMatrix[16],
			    const GLfloat projMatrix[16], 
			    const GLint viewport[4])
{
    r->useGLMatrices(modelMatrix, projMatrix, viewport);
}

void GLAPIENTRY
gluNurbsProperty(GLUnurbs *r, GLenum property, GLfloat value)
{
    GLfloat nurbsValue;
    
    switch (property) {
      case GLU_AUTO_LOAD_MATRIX:      
        r->setautoloadmode(value);
	return;

      case GLU_CULLING:
	if (value != 0.0) {
	    nurbsValue = N_CULLINGON;
	} else {
	    nurbsValue = N_NOCULLING;
	}
	r->setnurbsproperty(GL_MAP2_VERTEX_3, N_CULLING, nurbsValue);
	r->setnurbsproperty(GL_MAP2_VERTEX_4, N_CULLING, nurbsValue);
	r->setnurbsproperty(GL_MAP1_VERTEX_3, N_CULLING, nurbsValue);
	r->setnurbsproperty(GL_MAP1_VERTEX_4, N_CULLING, nurbsValue);
        return;

      case GLU_SAMPLING_METHOD:
	if (value == GLU_PATH_LENGTH) {
	    nurbsValue = N_PATHLENGTH;
	} else if (value == GLU_PARAMETRIC_ERROR) {
	    nurbsValue = N_PARAMETRICDISTANCE;
	} else if (value == GLU_DOMAIN_DISTANCE) {
	    nurbsValue = N_DOMAINDISTANCE;
            r->set_is_domain_distance_sampling(1); //optimzing untrimmed case

	} else if (value == GLU_OBJECT_PARAMETRIC_ERROR) {
	    nurbsValue = N_OBJECTSPACE_PARA;
	    r->setautoloadmode( 0.0 ); 
	    r->setSamplingMatrixIdentity();
	} else if (value == GLU_OBJECT_PATH_LENGTH) {
	    nurbsValue = N_OBJECTSPACE_PATH;
	    r->setautoloadmode( 0.0 ); 
	    r->setSamplingMatrixIdentity();
	} else {
            r->postError(GLU_INVALID_VALUE);
            return;
        }

	r->setnurbsproperty(GL_MAP2_VERTEX_3, N_SAMPLINGMETHOD, nurbsValue);
	r->setnurbsproperty(GL_MAP2_VERTEX_4, N_SAMPLINGMETHOD, nurbsValue);
	r->setnurbsproperty(GL_MAP1_VERTEX_3, N_SAMPLINGMETHOD, nurbsValue);
	r->setnurbsproperty(GL_MAP1_VERTEX_4, N_SAMPLINGMETHOD, nurbsValue);
	return;

      case GLU_SAMPLING_TOLERANCE:
	r->setnurbsproperty(GL_MAP2_VERTEX_3, N_PIXEL_TOLERANCE, value);
	r->setnurbsproperty(GL_MAP2_VERTEX_4, N_PIXEL_TOLERANCE, value);
	r->setnurbsproperty(GL_MAP1_VERTEX_3, N_PIXEL_TOLERANCE, value);
	r->setnurbsproperty(GL_MAP1_VERTEX_4, N_PIXEL_TOLERANCE, value);
	return;

      case GLU_PARAMETRIC_TOLERANCE:
	r->setnurbsproperty(GL_MAP2_VERTEX_3, N_ERROR_TOLERANCE, value);
        r->setnurbsproperty(GL_MAP2_VERTEX_4, N_ERROR_TOLERANCE, value);
        r->setnurbsproperty(GL_MAP1_VERTEX_3, N_ERROR_TOLERANCE, value);
        r->setnurbsproperty(GL_MAP1_VERTEX_4, N_ERROR_TOLERANCE, value);
        return;
	

      case GLU_DISPLAY_MODE:
	
	if (value == GLU_FILL) {
	  nurbsValue = N_FILL;
	} else if (value == GLU_OUTLINE_POLYGON) {
	  nurbsValue = N_OUTLINE_POLY;
	} else if (value == GLU_OUTLINE_PATCH) {
	  nurbsValue = N_OUTLINE_PATCH;
	} else {
	  r->postError(GLU_INVALID_VALUE);
	  return;
	}
	r->setnurbsproperty(N_DISPLAY, nurbsValue);
	
	break;

      case GLU_U_STEP:
    	r->setnurbsproperty(GL_MAP1_VERTEX_3, N_S_STEPS, value);
    	r->setnurbsproperty(GL_MAP1_VERTEX_4, N_S_STEPS, value);
    	r->setnurbsproperty(GL_MAP2_VERTEX_3, N_S_STEPS, value);
    	r->setnurbsproperty(GL_MAP2_VERTEX_4, N_S_STEPS, value);
	
	//added for optimizing untrimmed case
        r->set_domain_distance_u_rate(value);
	break;

      case GLU_V_STEP:
        r->setnurbsproperty(GL_MAP1_VERTEX_3, N_T_STEPS, value);
        r->setnurbsproperty(GL_MAP1_VERTEX_4, N_T_STEPS, value);
        r->setnurbsproperty(GL_MAP2_VERTEX_3, N_T_STEPS, value);
        r->setnurbsproperty(GL_MAP2_VERTEX_4, N_T_STEPS, value);

	//added for optimizing untrimmed case
        r->set_domain_distance_v_rate(value);
	break;
	
      case GLU_NURBS_MODE:
	if(value == GLU_NURBS_RENDERER)
	  r->put_callbackFlag(0);
	else if(value == GLU_NURBS_TESSELLATOR)
	  r->put_callbackFlag(1);
	else
	  r->postError(GLU_INVALID_ENUM);
	break;

      default:
	r->postError(GLU_INVALID_ENUM);
	return;	
    }
}

void GLAPIENTRY
gluGetNurbsProperty(GLUnurbs *r, GLenum property, GLfloat *value)
{
    GLfloat nurbsValue;

    switch(property) {
      case GLU_AUTO_LOAD_MATRIX:
	if (r->getautoloadmode()) {
	    *value = GL_TRUE;
	} else {
	    *value = GL_FALSE;
	}
	break;
      case GLU_CULLING:
	r->getnurbsproperty(GL_MAP2_VERTEX_3, N_CULLING, &nurbsValue);
	if (nurbsValue == N_CULLINGON) {
	    *value = GL_TRUE;
	} else {
	    *value = GL_FALSE;
	}
	break;
      case GLU_SAMPLING_METHOD:
	r->getnurbsproperty(GL_MAP2_VERTEX_3, N_SAMPLINGMETHOD, value);
	if(*value == N_PATHLENGTH)
	  *value = GLU_PATH_LENGTH;
	else if(*value == N_PARAMETRICDISTANCE)
	  *value = GLU_PARAMETRIC_ERROR;
	else if(*value == N_DOMAINDISTANCE)
	  *value = GLU_DOMAIN_DISTANCE;
	else if(*value == N_OBJECTSPACE_PATH)
	  *value = GLU_OBJECT_PATH_LENGTH;
	else if(*value == N_OBJECTSPACE_PARA)
	  *value = GLU_OBJECT_PARAMETRIC_ERROR;	
	break;
      case GLU_SAMPLING_TOLERANCE:
	r->getnurbsproperty(GL_MAP2_VERTEX_3, N_PIXEL_TOLERANCE, value);
	break;
      case GLU_PARAMETRIC_TOLERANCE:
	r->getnurbsproperty(GL_MAP2_VERTEX_3, N_ERROR_TOLERANCE, value);
        break;

      case GLU_U_STEP:
    	r->getnurbsproperty(GL_MAP2_VERTEX_3, N_S_STEPS, value);
	break;
      case GLU_V_STEP:
    	r->getnurbsproperty(GL_MAP2_VERTEX_3, N_T_STEPS, value);
	break;
      case GLU_DISPLAY_MODE:
	r->getnurbsproperty(N_DISPLAY, &nurbsValue);
	if (nurbsValue == N_FILL) {
	    *value = GLU_FILL;
	} else if (nurbsValue == N_OUTLINE_POLY) {
	    *value = GLU_OUTLINE_POLYGON;
	} else {
	    *value = GLU_OUTLINE_PATCH;
	}
	break;

      case GLU_NURBS_MODE:
	if(r->is_callback())
	  *value = GLU_NURBS_TESSELLATOR;
	else
	  *value = GLU_NURBS_RENDERER;
	break;
	
      default:
	r->postError(GLU_INVALID_ENUM);
	return;
    }
}

extern "C" void GLAPIENTRY
gluNurbsCallback(GLUnurbs *r, GLenum which, _GLUfuncptr fn )
{
    switch (which) {
    case GLU_NURBS_BEGIN:
    case GLU_NURBS_END:
    case GLU_NURBS_VERTEX:
    case GLU_NURBS_NORMAL:
    case GLU_NURBS_TEXTURE_COORD:
    case GLU_NURBS_COLOR:
    case GLU_NURBS_BEGIN_DATA:
    case GLU_NURBS_END_DATA:
    case GLU_NURBS_VERTEX_DATA:
    case GLU_NURBS_NORMAL_DATA:
    case GLU_NURBS_TEXTURE_COORD_DATA:
    case GLU_NURBS_COLOR_DATA: 
	r->putSurfCallBack(which, fn);
	break;

    case GLU_NURBS_ERROR:
	r->errorCallback = (void (APIENTRY *)( GLenum e )) fn;
	break;
    default:
	r->postError(GLU_INVALID_ENUM);
	return;
    }
}

extern "C"
void GLAPIENTRY
gluNurbsCallbackDataEXT(GLUnurbs* r, void* userData)
{
  r->setNurbsCallbackData(userData);
}

extern "C"
void GLAPIENTRY
gluNurbsCallbackData(GLUnurbs* r, void* userData)
{
  gluNurbsCallbackDataEXT(r,userData);
}
