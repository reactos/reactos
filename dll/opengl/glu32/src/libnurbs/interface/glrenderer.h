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
 * glrenderer.h
 *
 */

#ifndef __gluglrenderer_h_
#define __gluglrenderer_h_

#include <GL/gl.h>
#include <GL/glu.h>
#include "nurbstess.h"
#include "glsurfeval.h"
#include "glcurveval.h"

extern "C" {
      typedef void (APIENTRY *errorCallbackType)( GLenum );
}

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

    errorCallbackType errorCallback;
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
