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
 * glrenderer.h
 *
 * $Date$ $Revision: 1.1 $
 * $Header: /cygdrive/c/RCVS/CVS/ReactOS/reactos/lib/glu32/libnurbs/interface/glrenderer.h,v 1.1 2004/02/02 16:39:08 navaraf Exp $
 */

#ifndef __gluglrenderer_h_
#define __gluglrenderer_h_

#include <GL/gl.h>
#include <GL/glu.h>
#include "nurbstess.h"
#include "glsurfeval.h"
#include "glcurveval.h"

class GLUnurbs : public NurbsTessellator {

public:
		GLUnurbs( void );
    void 	loadGLMatrices( void );
    void	useGLMatrices( const GLfloat modelMatrix[16],
			       const GLfloat projMatrix[16],
			       const GLint viewport[4] );
    void		setSamplingMatrixIdentity( void );

    void 	errorHandler( int );
    void	bgnrender( void );
    void	endrender( void );
    void	setautoloadmode( INREAL value )
      		    {

		      if (value) autoloadmode = GL_TRUE;
		      else autoloadmode = GL_FALSE;

		    }
    GLboolean	getautoloadmode( void ) { return autoloadmode; }

    void	(APIENTRY *errorCallback)( GLenum );
    void	postError( int which )
		    { if (errorCallback) (errorCallback)( (GLenum)which ); }
#ifdef _WIN32
    void        putSurfCallBack(GLenum which, void (GLAPIENTRY *fn)() )
#else
    void        putSurfCallBack(GLenum which, _GLUfuncptr fn )
#endif
      {
	curveEvaluator.putCallBack(which, fn);
	surfaceEvaluator.putCallBack(which, fn);
      }

    int         get_vertices_call_back()
      {
	return surfaceEvaluator.get_vertices_call_back();
      }

    void        put_vertices_call_back(int flag)
      {
	surfaceEvaluator.put_vertices_call_back(flag);
      }

    int         get_callback_auto_normal()
      {
        return surfaceEvaluator.get_callback_auto_normal();
      }

    void         put_callback_auto_normal(int flag)
      {
        surfaceEvaluator.put_callback_auto_normal(flag);
      }

    void       setNurbsCallbackData(void* userData)
      {
       curveEvaluator.set_callback_userData(userData);
       surfaceEvaluator.set_callback_userData(userData);
     }


    //for LOD
    void LOD_eval_list(int level)
      {
	surfaceEvaluator.LOD_eval_list(level);
      }

    //NEWCALLBACK
    int        is_callback()
      {
	return callbackFlag;
      }
    void       put_callbackFlag(int flag)
      {
	callbackFlag = flag;
	surfaceEvaluator.put_vertices_call_back(flag);
	curveEvaluator.put_vertices_call_back(flag);
      }

private:
    GLboolean			autoloadmode;
    OpenGLSurfaceEvaluator	surfaceEvaluator;
    OpenGLCurveEvaluator	curveEvaluator;

    void		loadSamplingMatrix( const GLfloat vmat[4][4],
			        const GLint viewport[4] );
    void		loadCullingMatrix( GLfloat vmat[4][4] );
    static void		grabGLMatrix( GLfloat vmat[4][4] );
    static void		transform4d( GLfloat A[4], GLfloat B[4],
				GLfloat mat[4][4] );
    static void		multmatrix4d( GLfloat n[4][4], const GLfloat left[4][4],
				const GLfloat right[4][4] );

   int                  callbackFlag;
};

#endif /* __gluglrenderer_h_ */
