/*
 * Mesa 3-D graphics library
 * Version:  4.0
 * Copyright (C) 1995-2001  Brian Paul
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */


/*
 * FXMesa - 3Dfx Glide driver for Mesa.  Contributed by David Bucciarelli
 *
 * NOTE: This version requires Glide3 (http://sourceforge.net/projects/glide)
 */


#ifndef FXMESA_H
#define FXMESA_H


#include <glide.h>


#ifdef __cplusplus
extern "C" {
#endif


#define FXMESA_MAJOR_VERSION 6
#define FXMESA_MINOR_VERSION 3


/*
 * Values for attribList parameter to fxMesaCreateContext():
 */
#define FXMESA_NONE		0	/* to terminate attribList */
#define FXMESA_DOUBLEBUFFER	10
#define FXMESA_ALPHA_SIZE	11      /* followed by an integer */
#define FXMESA_DEPTH_SIZE	12      /* followed by an integer */
#define FXMESA_STENCIL_SIZE	13      /* followed by an integer */
#define FXMESA_ACCUM_SIZE	14      /* followed by an integer */
#define FXMESA_COLORDEPTH	20      /* followed by an integer */
#define FXMESA_SHARE_CONTEXT 990099	/* keep in sync with xmesa1.c! */



typedef struct tfxMesaContext *fxMesaContext;


#if defined (__BEOS__)
#pragma export on
#endif


GLAPI fxMesaContext GLAPIENTRY fxMesaCreateContext(GLuint win, GrScreenResolution_t,
						  GrScreenRefresh_t,
						  const GLint attribList[]);

GLAPI fxMesaContext GLAPIENTRY fxMesaCreateBestContext(GLuint win,
						      GLint width, GLint height,
						      const GLint attribList[]);
GLAPI void GLAPIENTRY fxMesaDestroyContext(fxMesaContext ctx);

GLAPI GLint GLAPIENTRY fxMesaSelectCurrentBoard(int n);

GLAPI void GLAPIENTRY fxMesaMakeCurrent(fxMesaContext ctx);

GLAPI fxMesaContext GLAPIENTRY fxMesaGetCurrentContext(void);

GLAPI void GLAPIENTRY fxMesaSwapBuffers(void);

GLAPI void GLAPIENTRY fxMesaSetNearFar(GLfloat nearVal, GLfloat farVal);

GLAPI void GLAPIENTRY fxMesaUpdateScreenSize(fxMesaContext ctx);

GLAPI void GLAPIENTRY fxCloseHardware(void);

GLAPI void GLAPIENTRY fxGetScreenGeometry (GLint *w, GLint *h);


#if defined (__BEOS__)
#pragma export off
#endif


#ifdef __cplusplus
}
#endif


#endif
