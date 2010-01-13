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
#include "glimports.h"
#include "glrenderer.h"

GLUnurbs::GLUnurbs()
	: NurbsTessellator(curveEvaluator, surfaceEvaluator)
{
    redefineMaps();
    defineMap(GL_MAP2_NORMAL, 0, 3);
    defineMap(GL_MAP1_NORMAL, 0, 3);
    defineMap(GL_MAP2_TEXTURE_COORD_1, 0, 1);
    defineMap(GL_MAP1_TEXTURE_COORD_1, 0, 1);
    defineMap(GL_MAP2_TEXTURE_COORD_2, 0, 2);
    defineMap(GL_MAP1_TEXTURE_COORD_2, 0, 2);
    defineMap(GL_MAP2_TEXTURE_COORD_3, 0, 3);
    defineMap(GL_MAP1_TEXTURE_COORD_3, 0, 3);
    defineMap(GL_MAP2_TEXTURE_COORD_4, 1, 4);
    defineMap(GL_MAP1_TEXTURE_COORD_4, 1, 4);
    defineMap(GL_MAP2_VERTEX_4, 1, 4);
    defineMap(GL_MAP1_VERTEX_4, 1, 4);
    defineMap(GL_MAP2_VERTEX_3, 0, 3);
    defineMap(GL_MAP1_VERTEX_3, 0, 3);
    defineMap(GL_MAP2_COLOR_4, 0, 4);
    defineMap(GL_MAP1_COLOR_4, 0, 4);
    defineMap(GL_MAP2_INDEX, 0, 1);
    defineMap(GL_MAP1_INDEX, 0, 1);

    setnurbsproperty(GL_MAP1_VERTEX_3, N_SAMPLINGMETHOD, (float) N_PATHLENGTH);
    setnurbsproperty(GL_MAP1_VERTEX_4, N_SAMPLINGMETHOD, (float) N_PATHLENGTH);
    setnurbsproperty(GL_MAP2_VERTEX_3, N_SAMPLINGMETHOD, (float) N_PATHLENGTH);
    setnurbsproperty(GL_MAP2_VERTEX_4, N_SAMPLINGMETHOD, (float) N_PATHLENGTH);

    setnurbsproperty(GL_MAP1_VERTEX_3, N_PIXEL_TOLERANCE, (float) 50.0);
    setnurbsproperty(GL_MAP1_VERTEX_4, N_PIXEL_TOLERANCE, (float) 50.0);
    setnurbsproperty(GL_MAP2_VERTEX_3, N_PIXEL_TOLERANCE, (float) 50.0);
    setnurbsproperty(GL_MAP2_VERTEX_4, N_PIXEL_TOLERANCE, (float) 50.0);

    setnurbsproperty(GL_MAP1_VERTEX_3, N_ERROR_TOLERANCE, (float) 0.50);
    setnurbsproperty(GL_MAP1_VERTEX_4, N_ERROR_TOLERANCE, (float) 0.50);
    setnurbsproperty(GL_MAP2_VERTEX_3, N_ERROR_TOLERANCE, (float) 0.50);
    setnurbsproperty(GL_MAP2_VERTEX_4, N_ERROR_TOLERANCE, (float) 0.50);

    setnurbsproperty(GL_MAP1_VERTEX_3, N_S_STEPS, (float) 100.0);
    setnurbsproperty(GL_MAP1_VERTEX_4, N_S_STEPS, (float) 100.0);
    setnurbsproperty(GL_MAP2_VERTEX_3, N_S_STEPS, (float) 100.0);
    setnurbsproperty(GL_MAP2_VERTEX_4, N_S_STEPS, (float) 100.0);

    //added for optimizing untrimmed case
    set_domain_distance_u_rate(100.0);

    setnurbsproperty(GL_MAP1_VERTEX_3, N_T_STEPS, (float) 100.0);
    setnurbsproperty(GL_MAP1_VERTEX_4, N_T_STEPS, (float) 100.0);
    setnurbsproperty(GL_MAP2_VERTEX_3, N_T_STEPS, (float) 100.0);
    setnurbsproperty(GL_MAP2_VERTEX_4, N_T_STEPS, (float) 100.0);

    //added for optimizing untrimmed case
    set_domain_distance_v_rate(100.0);
    set_is_domain_distance_sampling(0); //since the default is path_length

    //default autoloadmode is true
    autoloadmode = 1;

    //default callbackFlag is 0
    callbackFlag = 0;

    errorCallback = NULL;
}

void
GLUnurbs::bgnrender(void)
{
    if (autoloadmode) {
	loadGLMatrices();
    }
}

void
GLUnurbs::endrender(void)
{
}

void
GLUnurbs::errorHandler(int i)
{
    int gluError;

    gluError = i + (GLU_NURBS_ERROR1 - 1);
    postError( gluError );
}

void
GLUnurbs::loadGLMatrices(void)
{
    GLfloat vmat[4][4];
    GLint viewport[4];

    grabGLMatrix((GLfloat (*)[4]) vmat);
    loadCullingMatrix((GLfloat (*)[4]) vmat);
    ::glGetIntegerv((GLenum) GL_VIEWPORT, (GLint *) viewport);
    loadSamplingMatrix((const GLfloat (*)[4]) vmat, (const GLint *) viewport);
}

void
GLUnurbs::useGLMatrices(const GLfloat modelMatrix[16],
			  const GLfloat projMatrix[16],
			  const GLint viewport[4])
{
    GLfloat vmat[4][4];

    multmatrix4d(vmat, (const GLfloat (*)[4]) modelMatrix,
	    (const GLfloat (*)[4]) projMatrix);
    loadCullingMatrix((GLfloat (*)[4]) vmat);
    loadSamplingMatrix((const GLfloat (*)[4]) vmat, (const GLint *) viewport);
}

/*--------------------------------------------------------------------------
 * grabGLMatrix
 *--------------------------------------------------------------------------
 */

void
GLUnurbs::grabGLMatrix(GLfloat vmat[4][4])
{
    GLfloat m1[4][4], m2[4][4];

    ::glGetFloatv((GLenum) GL_MODELVIEW_MATRIX, (GLfloat *) &(m1[0][0]));
    ::glGetFloatv((GLenum) GL_PROJECTION_MATRIX, (GLfloat *) &(m2[0][0]));
    multmatrix4d((GLfloat (*)[4]) vmat,
	    (const GLfloat (*)[4]) m1, (const GLfloat (*)[4]) m2);
}

//for object space tesselation: view independent
void
GLUnurbs::setSamplingMatrixIdentity( void )
{
  INREAL smat[4][4] = {
    {1,0,0,0},
    {0,1,0,0},
    {0,0,1,0},
    {0,0,0,1}
  };
  const long rstride = sizeof(smat[0]) / sizeof(smat[0][0]);
  const long cstride = 1;

  setnurbsproperty(GL_MAP1_VERTEX_3, N_SAMPLINGMATRIX, &smat[0][0], rstride,
		   cstride);
  setnurbsproperty(GL_MAP1_VERTEX_4, N_SAMPLINGMATRIX, &smat[0][0], rstride,
		   cstride);
  setnurbsproperty(GL_MAP2_VERTEX_3, N_SAMPLINGMATRIX, &smat[0][0], rstride,
		   cstride);
  setnurbsproperty(GL_MAP2_VERTEX_4, N_SAMPLINGMATRIX, &smat[0][0], rstride,
		   cstride);
}


void
GLUnurbs::loadSamplingMatrix(const GLfloat vmat[4][4],
			       const GLint viewport[4])
{

    /* rescale the mapping to correspond to pixels in x/y */
    REAL xsize = 0.5 * (REAL) (viewport[2]);
    REAL ysize = 0.5 * (REAL) (viewport[3]);

    INREAL smat[4][4];
    smat[0][0] = vmat[0][0] * xsize;
    smat[1][0] = vmat[1][0] * xsize;
    smat[2][0] = vmat[2][0] * xsize;
    smat[3][0] = vmat[3][0] * xsize;

    smat[0][1] = vmat[0][1] * ysize;
    smat[1][1] = vmat[1][1] * ysize;
    smat[2][1] = vmat[2][1] * ysize;
    smat[3][1] = vmat[3][1] * ysize;

    smat[0][2] = 0.0;
    smat[1][2] = 0.0;
    smat[2][2] = 0.0;
    smat[3][2] = 0.0;

    smat[0][3] = vmat[0][3];
    smat[1][3] = vmat[1][3];
    smat[2][3] = vmat[2][3];
    smat[3][3] = vmat[3][3];

    const long rstride = sizeof(smat[0]) / sizeof(smat[0][0]);
    const long cstride = 1;

    setnurbsproperty(GL_MAP1_VERTEX_3, N_SAMPLINGMATRIX, &smat[0][0], rstride,
	    cstride);
    setnurbsproperty(GL_MAP1_VERTEX_4, N_SAMPLINGMATRIX, &smat[0][0], rstride,
	    cstride);
    setnurbsproperty(GL_MAP2_VERTEX_3, N_SAMPLINGMATRIX, &smat[0][0], rstride,
	    cstride);
    setnurbsproperty(GL_MAP2_VERTEX_4, N_SAMPLINGMATRIX, &smat[0][0], rstride,
	    cstride);
}

void
GLUnurbs::loadCullingMatrix(GLfloat vmat[4][4])
{
    INREAL cmat[4][4];

    cmat[0][0] = vmat[0][0];
    cmat[0][1] = vmat[0][1];
    cmat[0][2] = vmat[0][2];
    cmat[0][3] = vmat[0][3];

    cmat[1][0] = vmat[1][0];
    cmat[1][1] = vmat[1][1];
    cmat[1][2] = vmat[1][2];
    cmat[1][3] = vmat[1][3];

    cmat[2][0] = vmat[2][0];
    cmat[2][1] = vmat[2][1];
    cmat[2][2] = vmat[2][2];
    cmat[2][3] = vmat[2][3];

    cmat[3][0] = vmat[3][0];
    cmat[3][1] = vmat[3][1];
    cmat[3][2] = vmat[3][2];
    cmat[3][3] = vmat[3][3];

    const long rstride = sizeof(cmat[0]) / sizeof(cmat[0][0]);
    const long cstride = 1;

    setnurbsproperty(GL_MAP2_VERTEX_3, N_CULLINGMATRIX, &cmat[0][0], rstride,
	    cstride);
    setnurbsproperty(GL_MAP2_VERTEX_4, N_CULLINGMATRIX, &cmat[0][0], rstride,
	    cstride);
	//added for curves by zl
    setnurbsproperty(GL_MAP1_VERTEX_3, N_CULLINGMATRIX, &cmat[0][0], rstride,
	    cstride);
    setnurbsproperty(GL_MAP1_VERTEX_4, N_CULLINGMATRIX, &cmat[0][0], rstride,
	    cstride);
}

/*---------------------------------------------------------------------
 * A = B * MAT ; transform a 4d vector through a 4x4 matrix
 *---------------------------------------------------------------------
 */
void
GLUnurbs::transform4d(GLfloat A[4], GLfloat B[4], GLfloat mat[4][4])
{

    A[0] = B[0]*mat[0][0] + B[1]*mat[1][0] + B[2]*mat[2][0] + B[3]*mat[3][0];
    A[1] = B[0]*mat[0][1] + B[1]*mat[1][1] + B[2]*mat[2][1] + B[3]*mat[3][1];
    A[2] = B[0]*mat[0][2] + B[1]*mat[1][2] + B[2]*mat[2][2] + B[3]*mat[3][2];
    A[3] = B[0]*mat[0][3] + B[1]*mat[1][3] + B[2]*mat[2][3] + B[3]*mat[3][3];
}

/*---------------------------------------------------------------------
 * new = [left][right] ; multiply two matrices together
 *---------------------------------------------------------------------
 */
void
GLUnurbs::multmatrix4d (GLfloat n[4][4], const GLfloat left[4][4],
		 const GLfloat right[4][4])
{
    transform4d ((GLfloat *) n[0],(GLfloat *) left[0],(GLfloat (*)[4]) right);
    transform4d ((GLfloat *) n[1],(GLfloat *) left[1],(GLfloat (*)[4]) right);
    transform4d ((GLfloat *) n[2],(GLfloat *) left[2],(GLfloat (*)[4]) right);
    transform4d ((GLfloat *) n[3],(GLfloat *) left[3],(GLfloat (*)[4]) right);
}
