/**************************************************************************
 * 
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

#ifndef INTELTRIS_INC
#define INTELTRIS_INC

#include "mtypes.h"



#define _INTEL_NEW_RENDERSTATE (_DD_NEW_LINE_STIPPLE |		\
			       _DD_NEW_TRI_UNFILLED |		\
			       _DD_NEW_TRI_LIGHT_TWOSIDE |	\
			       _DD_NEW_TRI_OFFSET |		\
			       _DD_NEW_TRI_STIPPLE |		\
			       _NEW_PROGRAM |		\
			       _NEW_POLYGONSTIPPLE)

extern void intelInitTriFuncs(GLcontext * ctx);

extern void intelChooseRenderState(GLcontext * ctx);

extern void intelStartInlinePrimitive(struct intel_context *intel,
                                      GLuint prim, GLuint flags);
extern void intelWrapInlinePrimitive(struct intel_context *intel);

GLuint *intelExtendInlinePrimitive(struct intel_context *intel,
                                   GLuint dwords);


void intel_meta_draw_quad(struct intel_context *intel,
                          GLfloat x0, GLfloat x1,
                          GLfloat y0, GLfloat y1,
                          GLfloat z,
                          GLuint color,
                          GLfloat s0, GLfloat s1, GLfloat t0, GLfloat t1);

void intel_meta_draw_poly(struct intel_context *intel,
                          GLuint n,
                          GLfloat xy[][2],
                          GLfloat z, GLuint color, GLfloat tex[][2]);



#endif
