/* uglmesa.h - Public header UGL/Mesa */

/* Copyright (C) 2001 by Wind River Systems, Inc */

/*
 * Mesa 3-D graphics library
 * Version:  4.0
 *
 * The MIT License
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/*
 * Author:
 * Stephane Raimbault <stephane.raimbault@windriver.com> 
 */

#ifndef UGLMESA_H
#define UGLMESA_H

#ifdef __cplusplus
extern "C" {
#endif

#define UGL_MESA_MAJOR_VERSION 4
#define UGL_MESA_MINOR_VERSION 0

#include <GL/gl.h>
#include <ugl/ugl.h>

/*
 * Values for display mode of uglMesaCreateContext ()
 */

/*
 * With these mask values, it's possible to test double buffer mode
 * with UGL_MESA_DOUBLE mask
 *
 * SINGLE  0000 0001
 * DOUBLE  0000 0110
 * -  SOFT 0000 0010
 * -  HARD 0000 0100
 * WINDML  0001 0000
 *
 * 
 */
#define UGL_MESA_SINGLE            0x01
#define UGL_MESA_DOUBLE            0x06
#define UGL_MESA_DOUBLE_SOFTWARE   0x02
#define UGL_MESA_DOUBLE_HARDWARE   0x04
    
#define UGL_MESA_WINDML_EXCLUSIVE  0x10

#define UGL_MESA_FULLSCREEN_WIDTH  0x0
#define UGL_MESA_FULLSCREEN_HEIGHT 0x0

/*
 * uglMesaPixelStore() parameters:
 */
    
#define UGL_MESA_ROW_LENGTH	   0x20
#define UGL_MESA_Y_UP              0x21

/* 
 * Accepted by uglMesaGetIntegerv:
 */

#define UGL_MESA_LEFT_X		        0x01
#define UGL_MESA_TOP_Y		        0x02    
#define UGL_MESA_WIDTH		        0x03
#define UGL_MESA_HEIGHT		        0x04
#define UGL_MESA_DISPLAY_WIDTH          0x05
#define UGL_MESA_DISPLAY_HEIGHT         0x06
#define UGL_MESA_COLOR_FORMAT	        0x07
#define UGL_MESA_COLOR_MODEL            0x08
#define UGL_MESA_PIXEL_FORMAT           0x09
#define UGL_MESA_TYPE		        0x0A
#define UGL_MESA_RGB		        0x0B
#define UGL_MESA_COLOR_INDEXED          0x0C
#define UGL_MESA_SINGLE_BUFFER          0x0D
#define UGL_MESA_DOUBLE_BUFFER          0x0E
#define UGL_MESA_DOUBLE_BUFFER_SOFTWARE 0x0F
#define UGL_MESA_DOUBLE_BUFFER_HARDWARE 0x10
    
/*
 * typedefs
 */

typedef struct uglMesaContext * UGL_MESA_CONTEXT;
    
UGL_MESA_CONTEXT uglMesaCreateNewContext (GLenum mode,
					  UGL_MESA_CONTEXT share_list);

UGL_MESA_CONTEXT  uglMesaCreateNewContextExt (GLenum mode,
					      GLint depth_bits,
					      GLint stencil_bits,
					      GLint accum_red_bits,
					      GLint accum_green_bits,
					      GLint accum_blue_bits,
					      GLint accum_alpha_bits,
					      UGL_MESA_CONTEXT share_list);

GLboolean uglMesaMakeCurrentContext (UGL_MESA_CONTEXT umc,
				     GLsizei left, GLsizei top,
				     GLsizei width, GLsizei height);

GLboolean uglMesaMoveWindow (GLsizei dx, GLsizei dy);

GLboolean uglMesaMoveToWindow (GLsizei left, GLsizei top);

GLboolean uglMesaResizeWindow (GLsizei dw, GLsizei dh);

GLboolean uglMesaResizeToWindow (GLsizei width, GLsizei height);

void uglMesaDestroyContext (void);

UGL_MESA_CONTEXT uglMesaGetCurrentContext (void);

void uglMesaSwapBuffers (void);

void uglMesaPixelStore (GLint pname, GLint value);

void uglMesaGetIntegerv (GLint pname, GLint *value);

GLboolean uglMesaGetDepthBuffer (GLint *width, GLint *height,
				 GLint *bytesPerValue, void **buffer);

GLboolean uglMesaGetColorBuffer (GLint *width, GLint *height,
				 GLint *format, void **buffer);

GLboolean uglMesaSetColor (GLubyte index, GLfloat red,
			   GLfloat green, GLfloat blue);
  
#ifdef __cplusplus
}
#endif


#endif
