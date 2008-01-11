/* DO NOT EDIT - This file generated automatically by gl_SPARC_asm.py (from Mesa) script */

/*
 * Copyright (C) 1999-2003  Brian Paul   All Rights Reserved.
 * (C) Copyright IBM Corporation 2004
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sub license,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL, IBM,
 * AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "glapioffsets.h"

#ifdef __arch64__
#  define GL_STUB(fn,off)				\
fn:					\
	sethi	%hi(0xDEADBEEF), %g4 ;			\
	sethi	%hi(0xDEADBEEF), %g1 ;			\
	or	%g4, %lo(0xDEADBEEF), %g4 ;		\
	or	%g1, %lo(0xDEADBEEF), %g1 ;		\
	sllx	%g4, 32, %g4 ;				\
	ldx	[%g1 + %g4], %g1 ;			\
	sethi	%hi(8 * off), %g4 ;			\
	or	%g4, %lo(8 * off), %g4 ;		\
	ldx	[%g1 + %g4], %g5 ;			\
	jmpl	%g5, %g0 ;				\
	nop
#else
#  define GL_STUB(fn,off)				\
fn:					\
	sethi	%hi(0xDEADBEEF), %g1 ;			\
	ld	[%g1 + %lo(0xDEADBEEF)], %g1 ;		\
	ld	[%g1 + (4 * off)], %g5 ;		\
	jmpl	%g5, %g0 ;				\
	nop
#endif

#define GL_STUB_ALIAS(fn,alias) fn = alias

.text
.align 32
		.globl __glapi_sparc_icache_flush ; .type __glapi_sparc_icache_flush,#function
__glapi_sparc_icache_flush: /* %o0 = insn_addr */
	flush	%o0
	retl
	nop

.data
.align 64

		.globl glNewList ; .type glNewList,#function
		.globl glEndList ; .type glEndList,#function
		.globl glCallList ; .type glCallList,#function
		.globl glCallLists ; .type glCallLists,#function
		.globl glDeleteLists ; .type glDeleteLists,#function
		.globl glGenLists ; .type glGenLists,#function
		.globl glListBase ; .type glListBase,#function
		.globl glBegin ; .type glBegin,#function
		.globl glBitmap ; .type glBitmap,#function
		.globl glColor3b ; .type glColor3b,#function
		.globl glColor3bv ; .type glColor3bv,#function
		.globl glColor3d ; .type glColor3d,#function
		.globl glColor3dv ; .type glColor3dv,#function
		.globl glColor3f ; .type glColor3f,#function
		.globl glColor3fv ; .type glColor3fv,#function
		.globl glColor3i ; .type glColor3i,#function
		.globl glColor3iv ; .type glColor3iv,#function
		.globl glColor3s ; .type glColor3s,#function
		.globl glColor3sv ; .type glColor3sv,#function
		.globl glColor3ub ; .type glColor3ub,#function
		.globl glColor3ubv ; .type glColor3ubv,#function
		.globl glColor3ui ; .type glColor3ui,#function
		.globl glColor3uiv ; .type glColor3uiv,#function
		.globl glColor3us ; .type glColor3us,#function
		.globl glColor3usv ; .type glColor3usv,#function
		.globl glColor4b ; .type glColor4b,#function
		.globl glColor4bv ; .type glColor4bv,#function
		.globl glColor4d ; .type glColor4d,#function
		.globl glColor4dv ; .type glColor4dv,#function
		.globl glColor4f ; .type glColor4f,#function
		.globl glColor4fv ; .type glColor4fv,#function
		.globl glColor4i ; .type glColor4i,#function
		.globl glColor4iv ; .type glColor4iv,#function
		.globl glColor4s ; .type glColor4s,#function
		.globl glColor4sv ; .type glColor4sv,#function
		.globl glColor4ub ; .type glColor4ub,#function
		.globl glColor4ubv ; .type glColor4ubv,#function
		.globl glColor4ui ; .type glColor4ui,#function
		.globl glColor4uiv ; .type glColor4uiv,#function
		.globl glColor4us ; .type glColor4us,#function
		.globl glColor4usv ; .type glColor4usv,#function
		.globl glEdgeFlag ; .type glEdgeFlag,#function
		.globl glEdgeFlagv ; .type glEdgeFlagv,#function
		.globl glEnd ; .type glEnd,#function
		.globl glIndexd ; .type glIndexd,#function
		.globl glIndexdv ; .type glIndexdv,#function
		.globl glIndexf ; .type glIndexf,#function
		.globl glIndexfv ; .type glIndexfv,#function
		.globl glIndexi ; .type glIndexi,#function
		.globl glIndexiv ; .type glIndexiv,#function
		.globl glIndexs ; .type glIndexs,#function
		.globl glIndexsv ; .type glIndexsv,#function
		.globl glNormal3b ; .type glNormal3b,#function
		.globl glNormal3bv ; .type glNormal3bv,#function
		.globl glNormal3d ; .type glNormal3d,#function
		.globl glNormal3dv ; .type glNormal3dv,#function
		.globl glNormal3f ; .type glNormal3f,#function
		.globl glNormal3fv ; .type glNormal3fv,#function
		.globl glNormal3i ; .type glNormal3i,#function
		.globl glNormal3iv ; .type glNormal3iv,#function
		.globl glNormal3s ; .type glNormal3s,#function
		.globl glNormal3sv ; .type glNormal3sv,#function
		.globl glRasterPos2d ; .type glRasterPos2d,#function
		.globl glRasterPos2dv ; .type glRasterPos2dv,#function
		.globl glRasterPos2f ; .type glRasterPos2f,#function
		.globl glRasterPos2fv ; .type glRasterPos2fv,#function
		.globl glRasterPos2i ; .type glRasterPos2i,#function
		.globl glRasterPos2iv ; .type glRasterPos2iv,#function
		.globl glRasterPos2s ; .type glRasterPos2s,#function
		.globl glRasterPos2sv ; .type glRasterPos2sv,#function
		.globl glRasterPos3d ; .type glRasterPos3d,#function
		.globl glRasterPos3dv ; .type glRasterPos3dv,#function
		.globl glRasterPos3f ; .type glRasterPos3f,#function
		.globl glRasterPos3fv ; .type glRasterPos3fv,#function
		.globl glRasterPos3i ; .type glRasterPos3i,#function
		.globl glRasterPos3iv ; .type glRasterPos3iv,#function
		.globl glRasterPos3s ; .type glRasterPos3s,#function
		.globl glRasterPos3sv ; .type glRasterPos3sv,#function
		.globl glRasterPos4d ; .type glRasterPos4d,#function
		.globl glRasterPos4dv ; .type glRasterPos4dv,#function
		.globl glRasterPos4f ; .type glRasterPos4f,#function
		.globl glRasterPos4fv ; .type glRasterPos4fv,#function
		.globl glRasterPos4i ; .type glRasterPos4i,#function
		.globl glRasterPos4iv ; .type glRasterPos4iv,#function
		.globl glRasterPos4s ; .type glRasterPos4s,#function
		.globl glRasterPos4sv ; .type glRasterPos4sv,#function
		.globl glRectd ; .type glRectd,#function
		.globl glRectdv ; .type glRectdv,#function
		.globl glRectf ; .type glRectf,#function
		.globl glRectfv ; .type glRectfv,#function
		.globl glRecti ; .type glRecti,#function
		.globl glRectiv ; .type glRectiv,#function
		.globl glRects ; .type glRects,#function
		.globl glRectsv ; .type glRectsv,#function
		.globl glTexCoord1d ; .type glTexCoord1d,#function
		.globl glTexCoord1dv ; .type glTexCoord1dv,#function
		.globl glTexCoord1f ; .type glTexCoord1f,#function
		.globl glTexCoord1fv ; .type glTexCoord1fv,#function
		.globl glTexCoord1i ; .type glTexCoord1i,#function
		.globl glTexCoord1iv ; .type glTexCoord1iv,#function
		.globl glTexCoord1s ; .type glTexCoord1s,#function
		.globl glTexCoord1sv ; .type glTexCoord1sv,#function
		.globl glTexCoord2d ; .type glTexCoord2d,#function
		.globl glTexCoord2dv ; .type glTexCoord2dv,#function
		.globl glTexCoord2f ; .type glTexCoord2f,#function
		.globl glTexCoord2fv ; .type glTexCoord2fv,#function
		.globl glTexCoord2i ; .type glTexCoord2i,#function
		.globl glTexCoord2iv ; .type glTexCoord2iv,#function
		.globl glTexCoord2s ; .type glTexCoord2s,#function
		.globl glTexCoord2sv ; .type glTexCoord2sv,#function
		.globl glTexCoord3d ; .type glTexCoord3d,#function
		.globl glTexCoord3dv ; .type glTexCoord3dv,#function
		.globl glTexCoord3f ; .type glTexCoord3f,#function
		.globl glTexCoord3fv ; .type glTexCoord3fv,#function
		.globl glTexCoord3i ; .type glTexCoord3i,#function
		.globl glTexCoord3iv ; .type glTexCoord3iv,#function
		.globl glTexCoord3s ; .type glTexCoord3s,#function
		.globl glTexCoord3sv ; .type glTexCoord3sv,#function
		.globl glTexCoord4d ; .type glTexCoord4d,#function
		.globl glTexCoord4dv ; .type glTexCoord4dv,#function
		.globl glTexCoord4f ; .type glTexCoord4f,#function
		.globl glTexCoord4fv ; .type glTexCoord4fv,#function
		.globl glTexCoord4i ; .type glTexCoord4i,#function
		.globl glTexCoord4iv ; .type glTexCoord4iv,#function
		.globl glTexCoord4s ; .type glTexCoord4s,#function
		.globl glTexCoord4sv ; .type glTexCoord4sv,#function
		.globl glVertex2d ; .type glVertex2d,#function
		.globl glVertex2dv ; .type glVertex2dv,#function
		.globl glVertex2f ; .type glVertex2f,#function
		.globl glVertex2fv ; .type glVertex2fv,#function
		.globl glVertex2i ; .type glVertex2i,#function
		.globl glVertex2iv ; .type glVertex2iv,#function
		.globl glVertex2s ; .type glVertex2s,#function
		.globl glVertex2sv ; .type glVertex2sv,#function
		.globl glVertex3d ; .type glVertex3d,#function
		.globl glVertex3dv ; .type glVertex3dv,#function
		.globl glVertex3f ; .type glVertex3f,#function
		.globl glVertex3fv ; .type glVertex3fv,#function
		.globl glVertex3i ; .type glVertex3i,#function
		.globl glVertex3iv ; .type glVertex3iv,#function
		.globl glVertex3s ; .type glVertex3s,#function
		.globl glVertex3sv ; .type glVertex3sv,#function
		.globl glVertex4d ; .type glVertex4d,#function
		.globl glVertex4dv ; .type glVertex4dv,#function
		.globl glVertex4f ; .type glVertex4f,#function
		.globl glVertex4fv ; .type glVertex4fv,#function
		.globl glVertex4i ; .type glVertex4i,#function
		.globl glVertex4iv ; .type glVertex4iv,#function
		.globl glVertex4s ; .type glVertex4s,#function
		.globl glVertex4sv ; .type glVertex4sv,#function
		.globl glClipPlane ; .type glClipPlane,#function
		.globl glColorMaterial ; .type glColorMaterial,#function
		.globl glCullFace ; .type glCullFace,#function
		.globl glFogf ; .type glFogf,#function
		.globl glFogfv ; .type glFogfv,#function
		.globl glFogi ; .type glFogi,#function
		.globl glFogiv ; .type glFogiv,#function
		.globl glFrontFace ; .type glFrontFace,#function
		.globl glHint ; .type glHint,#function
		.globl glLightf ; .type glLightf,#function
		.globl glLightfv ; .type glLightfv,#function
		.globl glLighti ; .type glLighti,#function
		.globl glLightiv ; .type glLightiv,#function
		.globl glLightModelf ; .type glLightModelf,#function
		.globl glLightModelfv ; .type glLightModelfv,#function
		.globl glLightModeli ; .type glLightModeli,#function
		.globl glLightModeliv ; .type glLightModeliv,#function
		.globl glLineStipple ; .type glLineStipple,#function
		.globl glLineWidth ; .type glLineWidth,#function
		.globl glMaterialf ; .type glMaterialf,#function
		.globl glMaterialfv ; .type glMaterialfv,#function
		.globl glMateriali ; .type glMateriali,#function
		.globl glMaterialiv ; .type glMaterialiv,#function
		.globl glPointSize ; .type glPointSize,#function
		.globl glPolygonMode ; .type glPolygonMode,#function
		.globl glPolygonStipple ; .type glPolygonStipple,#function
		.globl glScissor ; .type glScissor,#function
		.globl glShadeModel ; .type glShadeModel,#function
		.globl glTexParameterf ; .type glTexParameterf,#function
		.globl glTexParameterfv ; .type glTexParameterfv,#function
		.globl glTexParameteri ; .type glTexParameteri,#function
		.globl glTexParameteriv ; .type glTexParameteriv,#function
		.globl glTexImage1D ; .type glTexImage1D,#function
		.globl glTexImage2D ; .type glTexImage2D,#function
		.globl glTexEnvf ; .type glTexEnvf,#function
		.globl glTexEnvfv ; .type glTexEnvfv,#function
		.globl glTexEnvi ; .type glTexEnvi,#function
		.globl glTexEnviv ; .type glTexEnviv,#function
		.globl glTexGend ; .type glTexGend,#function
		.globl glTexGendv ; .type glTexGendv,#function
		.globl glTexGenf ; .type glTexGenf,#function
		.globl glTexGenfv ; .type glTexGenfv,#function
		.globl glTexGeni ; .type glTexGeni,#function
		.globl glTexGeniv ; .type glTexGeniv,#function
		.globl glFeedbackBuffer ; .type glFeedbackBuffer,#function
		.globl glSelectBuffer ; .type glSelectBuffer,#function
		.globl glRenderMode ; .type glRenderMode,#function
		.globl glInitNames ; .type glInitNames,#function
		.globl glLoadName ; .type glLoadName,#function
		.globl glPassThrough ; .type glPassThrough,#function
		.globl glPopName ; .type glPopName,#function
		.globl glPushName ; .type glPushName,#function
		.globl glDrawBuffer ; .type glDrawBuffer,#function
		.globl glClear ; .type glClear,#function
		.globl glClearAccum ; .type glClearAccum,#function
		.globl glClearIndex ; .type glClearIndex,#function
		.globl glClearColor ; .type glClearColor,#function
		.globl glClearStencil ; .type glClearStencil,#function
		.globl glClearDepth ; .type glClearDepth,#function
		.globl glStencilMask ; .type glStencilMask,#function
		.globl glColorMask ; .type glColorMask,#function
		.globl glDepthMask ; .type glDepthMask,#function
		.globl glIndexMask ; .type glIndexMask,#function
		.globl glAccum ; .type glAccum,#function
		.globl glDisable ; .type glDisable,#function
		.globl glEnable ; .type glEnable,#function
		.globl glFinish ; .type glFinish,#function
		.globl glFlush ; .type glFlush,#function
		.globl glPopAttrib ; .type glPopAttrib,#function
		.globl glPushAttrib ; .type glPushAttrib,#function
		.globl glMap1d ; .type glMap1d,#function
		.globl glMap1f ; .type glMap1f,#function
		.globl glMap2d ; .type glMap2d,#function
		.globl glMap2f ; .type glMap2f,#function
		.globl glMapGrid1d ; .type glMapGrid1d,#function
		.globl glMapGrid1f ; .type glMapGrid1f,#function
		.globl glMapGrid2d ; .type glMapGrid2d,#function
		.globl glMapGrid2f ; .type glMapGrid2f,#function
		.globl glEvalCoord1d ; .type glEvalCoord1d,#function
		.globl glEvalCoord1dv ; .type glEvalCoord1dv,#function
		.globl glEvalCoord1f ; .type glEvalCoord1f,#function
		.globl glEvalCoord1fv ; .type glEvalCoord1fv,#function
		.globl glEvalCoord2d ; .type glEvalCoord2d,#function
		.globl glEvalCoord2dv ; .type glEvalCoord2dv,#function
		.globl glEvalCoord2f ; .type glEvalCoord2f,#function
		.globl glEvalCoord2fv ; .type glEvalCoord2fv,#function
		.globl glEvalMesh1 ; .type glEvalMesh1,#function
		.globl glEvalPoint1 ; .type glEvalPoint1,#function
		.globl glEvalMesh2 ; .type glEvalMesh2,#function
		.globl glEvalPoint2 ; .type glEvalPoint2,#function
		.globl glAlphaFunc ; .type glAlphaFunc,#function
		.globl glBlendFunc ; .type glBlendFunc,#function
		.globl glLogicOp ; .type glLogicOp,#function
		.globl glStencilFunc ; .type glStencilFunc,#function
		.globl glStencilOp ; .type glStencilOp,#function
		.globl glDepthFunc ; .type glDepthFunc,#function
		.globl glPixelZoom ; .type glPixelZoom,#function
		.globl glPixelTransferf ; .type glPixelTransferf,#function
		.globl glPixelTransferi ; .type glPixelTransferi,#function
		.globl glPixelStoref ; .type glPixelStoref,#function
		.globl glPixelStorei ; .type glPixelStorei,#function
		.globl glPixelMapfv ; .type glPixelMapfv,#function
		.globl glPixelMapuiv ; .type glPixelMapuiv,#function
		.globl glPixelMapusv ; .type glPixelMapusv,#function
		.globl glReadBuffer ; .type glReadBuffer,#function
		.globl glCopyPixels ; .type glCopyPixels,#function
		.globl glReadPixels ; .type glReadPixels,#function
		.globl glDrawPixels ; .type glDrawPixels,#function
		.globl glGetBooleanv ; .type glGetBooleanv,#function
		.globl glGetClipPlane ; .type glGetClipPlane,#function
		.globl glGetDoublev ; .type glGetDoublev,#function
		.globl glGetError ; .type glGetError,#function
		.globl glGetFloatv ; .type glGetFloatv,#function
		.globl glGetIntegerv ; .type glGetIntegerv,#function
		.globl glGetLightfv ; .type glGetLightfv,#function
		.globl glGetLightiv ; .type glGetLightiv,#function
		.globl glGetMapdv ; .type glGetMapdv,#function
		.globl glGetMapfv ; .type glGetMapfv,#function
		.globl glGetMapiv ; .type glGetMapiv,#function
		.globl glGetMaterialfv ; .type glGetMaterialfv,#function
		.globl glGetMaterialiv ; .type glGetMaterialiv,#function
		.globl glGetPixelMapfv ; .type glGetPixelMapfv,#function
		.globl glGetPixelMapuiv ; .type glGetPixelMapuiv,#function
		.globl glGetPixelMapusv ; .type glGetPixelMapusv,#function
		.globl glGetPolygonStipple ; .type glGetPolygonStipple,#function
		.globl glGetString ; .type glGetString,#function
		.globl glGetTexEnvfv ; .type glGetTexEnvfv,#function
		.globl glGetTexEnviv ; .type glGetTexEnviv,#function
		.globl glGetTexGendv ; .type glGetTexGendv,#function
		.globl glGetTexGenfv ; .type glGetTexGenfv,#function
		.globl glGetTexGeniv ; .type glGetTexGeniv,#function
		.globl glGetTexImage ; .type glGetTexImage,#function
		.globl glGetTexParameterfv ; .type glGetTexParameterfv,#function
		.globl glGetTexParameteriv ; .type glGetTexParameteriv,#function
		.globl glGetTexLevelParameterfv ; .type glGetTexLevelParameterfv,#function
		.globl glGetTexLevelParameteriv ; .type glGetTexLevelParameteriv,#function
		.globl glIsEnabled ; .type glIsEnabled,#function
		.globl glIsList ; .type glIsList,#function
		.globl glDepthRange ; .type glDepthRange,#function
		.globl glFrustum ; .type glFrustum,#function
		.globl glLoadIdentity ; .type glLoadIdentity,#function
		.globl glLoadMatrixf ; .type glLoadMatrixf,#function
		.globl glLoadMatrixd ; .type glLoadMatrixd,#function
		.globl glMatrixMode ; .type glMatrixMode,#function
		.globl glMultMatrixf ; .type glMultMatrixf,#function
		.globl glMultMatrixd ; .type glMultMatrixd,#function
		.globl glOrtho ; .type glOrtho,#function
		.globl glPopMatrix ; .type glPopMatrix,#function
		.globl glPushMatrix ; .type glPushMatrix,#function
		.globl glRotated ; .type glRotated,#function
		.globl glRotatef ; .type glRotatef,#function
		.globl glScaled ; .type glScaled,#function
		.globl glScalef ; .type glScalef,#function
		.globl glTranslated ; .type glTranslated,#function
		.globl glTranslatef ; .type glTranslatef,#function
		.globl glViewport ; .type glViewport,#function
		.globl glArrayElement ; .type glArrayElement,#function
		.globl glBindTexture ; .type glBindTexture,#function
		.globl glColorPointer ; .type glColorPointer,#function
		.globl glDisableClientState ; .type glDisableClientState,#function
		.globl glDrawArrays ; .type glDrawArrays,#function
		.globl glDrawElements ; .type glDrawElements,#function
		.globl glEdgeFlagPointer ; .type glEdgeFlagPointer,#function
		.globl glEnableClientState ; .type glEnableClientState,#function
		.globl glIndexPointer ; .type glIndexPointer,#function
		.globl glIndexub ; .type glIndexub,#function
		.globl glIndexubv ; .type glIndexubv,#function
		.globl glInterleavedArrays ; .type glInterleavedArrays,#function
		.globl glNormalPointer ; .type glNormalPointer,#function
		.globl glPolygonOffset ; .type glPolygonOffset,#function
		.globl glTexCoordPointer ; .type glTexCoordPointer,#function
		.globl glVertexPointer ; .type glVertexPointer,#function
		.globl glAreTexturesResident ; .type glAreTexturesResident,#function
		.globl glCopyTexImage1D ; .type glCopyTexImage1D,#function
		.globl glCopyTexImage2D ; .type glCopyTexImage2D,#function
		.globl glCopyTexSubImage1D ; .type glCopyTexSubImage1D,#function
		.globl glCopyTexSubImage2D ; .type glCopyTexSubImage2D,#function
		.globl glDeleteTextures ; .type glDeleteTextures,#function
		.globl glGenTextures ; .type glGenTextures,#function
		.globl glGetPointerv ; .type glGetPointerv,#function
		.globl glIsTexture ; .type glIsTexture,#function
		.globl glPrioritizeTextures ; .type glPrioritizeTextures,#function
		.globl glTexSubImage1D ; .type glTexSubImage1D,#function
		.globl glTexSubImage2D ; .type glTexSubImage2D,#function
		.globl glPopClientAttrib ; .type glPopClientAttrib,#function
		.globl glPushClientAttrib ; .type glPushClientAttrib,#function
		.globl glBlendColor ; .type glBlendColor,#function
		.globl glBlendEquation ; .type glBlendEquation,#function
		.globl glDrawRangeElements ; .type glDrawRangeElements,#function
		.globl glColorTable ; .type glColorTable,#function
		.globl glColorTableParameterfv ; .type glColorTableParameterfv,#function
		.globl glColorTableParameteriv ; .type glColorTableParameteriv,#function
		.globl glCopyColorTable ; .type glCopyColorTable,#function
		.globl glGetColorTable ; .type glGetColorTable,#function
		.globl glGetColorTableParameterfv ; .type glGetColorTableParameterfv,#function
		.globl glGetColorTableParameteriv ; .type glGetColorTableParameteriv,#function
		.globl glColorSubTable ; .type glColorSubTable,#function
		.globl glCopyColorSubTable ; .type glCopyColorSubTable,#function
		.globl glConvolutionFilter1D ; .type glConvolutionFilter1D,#function
		.globl glConvolutionFilter2D ; .type glConvolutionFilter2D,#function
		.globl glConvolutionParameterf ; .type glConvolutionParameterf,#function
		.globl glConvolutionParameterfv ; .type glConvolutionParameterfv,#function
		.globl glConvolutionParameteri ; .type glConvolutionParameteri,#function
		.globl glConvolutionParameteriv ; .type glConvolutionParameteriv,#function
		.globl glCopyConvolutionFilter1D ; .type glCopyConvolutionFilter1D,#function
		.globl glCopyConvolutionFilter2D ; .type glCopyConvolutionFilter2D,#function
		.globl glGetConvolutionFilter ; .type glGetConvolutionFilter,#function
		.globl glGetConvolutionParameterfv ; .type glGetConvolutionParameterfv,#function
		.globl glGetConvolutionParameteriv ; .type glGetConvolutionParameteriv,#function
		.globl glGetSeparableFilter ; .type glGetSeparableFilter,#function
		.globl glSeparableFilter2D ; .type glSeparableFilter2D,#function
		.globl glGetHistogram ; .type glGetHistogram,#function
		.globl glGetHistogramParameterfv ; .type glGetHistogramParameterfv,#function
		.globl glGetHistogramParameteriv ; .type glGetHistogramParameteriv,#function
		.globl glGetMinmax ; .type glGetMinmax,#function
		.globl glGetMinmaxParameterfv ; .type glGetMinmaxParameterfv,#function
		.globl glGetMinmaxParameteriv ; .type glGetMinmaxParameteriv,#function
		.globl glHistogram ; .type glHistogram,#function
		.globl glMinmax ; .type glMinmax,#function
		.globl glResetHistogram ; .type glResetHistogram,#function
		.globl glResetMinmax ; .type glResetMinmax,#function
		.globl glTexImage3D ; .type glTexImage3D,#function
		.globl glTexSubImage3D ; .type glTexSubImage3D,#function
		.globl glCopyTexSubImage3D ; .type glCopyTexSubImage3D,#function
		.globl glActiveTextureARB ; .type glActiveTextureARB,#function
		.globl glClientActiveTextureARB ; .type glClientActiveTextureARB,#function
		.globl glMultiTexCoord1dARB ; .type glMultiTexCoord1dARB,#function
		.globl glMultiTexCoord1dvARB ; .type glMultiTexCoord1dvARB,#function
		.globl glMultiTexCoord1fARB ; .type glMultiTexCoord1fARB,#function
		.globl glMultiTexCoord1fvARB ; .type glMultiTexCoord1fvARB,#function
		.globl glMultiTexCoord1iARB ; .type glMultiTexCoord1iARB,#function
		.globl glMultiTexCoord1ivARB ; .type glMultiTexCoord1ivARB,#function
		.globl glMultiTexCoord1sARB ; .type glMultiTexCoord1sARB,#function
		.globl glMultiTexCoord1svARB ; .type glMultiTexCoord1svARB,#function
		.globl glMultiTexCoord2dARB ; .type glMultiTexCoord2dARB,#function
		.globl glMultiTexCoord2dvARB ; .type glMultiTexCoord2dvARB,#function
		.globl glMultiTexCoord2fARB ; .type glMultiTexCoord2fARB,#function
		.globl glMultiTexCoord2fvARB ; .type glMultiTexCoord2fvARB,#function
		.globl glMultiTexCoord2iARB ; .type glMultiTexCoord2iARB,#function
		.globl glMultiTexCoord2ivARB ; .type glMultiTexCoord2ivARB,#function
		.globl glMultiTexCoord2sARB ; .type glMultiTexCoord2sARB,#function
		.globl glMultiTexCoord2svARB ; .type glMultiTexCoord2svARB,#function
		.globl glMultiTexCoord3dARB ; .type glMultiTexCoord3dARB,#function
		.globl glMultiTexCoord3dvARB ; .type glMultiTexCoord3dvARB,#function
		.globl glMultiTexCoord3fARB ; .type glMultiTexCoord3fARB,#function
		.globl glMultiTexCoord3fvARB ; .type glMultiTexCoord3fvARB,#function
		.globl glMultiTexCoord3iARB ; .type glMultiTexCoord3iARB,#function
		.globl glMultiTexCoord3ivARB ; .type glMultiTexCoord3ivARB,#function
		.globl glMultiTexCoord3sARB ; .type glMultiTexCoord3sARB,#function
		.globl glMultiTexCoord3svARB ; .type glMultiTexCoord3svARB,#function
		.globl glMultiTexCoord4dARB ; .type glMultiTexCoord4dARB,#function
		.globl glMultiTexCoord4dvARB ; .type glMultiTexCoord4dvARB,#function
		.globl glMultiTexCoord4fARB ; .type glMultiTexCoord4fARB,#function
		.globl glMultiTexCoord4fvARB ; .type glMultiTexCoord4fvARB,#function
		.globl glMultiTexCoord4iARB ; .type glMultiTexCoord4iARB,#function
		.globl glMultiTexCoord4ivARB ; .type glMultiTexCoord4ivARB,#function
		.globl glMultiTexCoord4sARB ; .type glMultiTexCoord4sARB,#function
		.globl glMultiTexCoord4svARB ; .type glMultiTexCoord4svARB,#function
		.globl glAttachShader ; .type glAttachShader,#function
		.globl glCreateProgram ; .type glCreateProgram,#function
		.globl glCreateShader ; .type glCreateShader,#function
		.globl glDeleteProgram ; .type glDeleteProgram,#function
		.globl glDeleteShader ; .type glDeleteShader,#function
		.globl glDetachShader ; .type glDetachShader,#function
		.globl glGetAttachedShaders ; .type glGetAttachedShaders,#function
		.globl glGetProgramInfoLog ; .type glGetProgramInfoLog,#function
		.globl glGetProgramiv ; .type glGetProgramiv,#function
		.globl glGetShaderInfoLog ; .type glGetShaderInfoLog,#function
		.globl glGetShaderiv ; .type glGetShaderiv,#function
		.globl glIsProgram ; .type glIsProgram,#function
		.globl glIsShader ; .type glIsShader,#function
		.globl glStencilFuncSeparate ; .type glStencilFuncSeparate,#function
		.globl glStencilMaskSeparate ; .type glStencilMaskSeparate,#function
		.globl glStencilOpSeparate ; .type glStencilOpSeparate,#function
		.globl glUniformMatrix2x3fv ; .type glUniformMatrix2x3fv,#function
		.globl glUniformMatrix2x4fv ; .type glUniformMatrix2x4fv,#function
		.globl glUniformMatrix3x2fv ; .type glUniformMatrix3x2fv,#function
		.globl glUniformMatrix3x4fv ; .type glUniformMatrix3x4fv,#function
		.globl glUniformMatrix4x2fv ; .type glUniformMatrix4x2fv,#function
		.globl glUniformMatrix4x3fv ; .type glUniformMatrix4x3fv,#function
		.globl glLoadTransposeMatrixdARB ; .type glLoadTransposeMatrixdARB,#function
		.globl glLoadTransposeMatrixfARB ; .type glLoadTransposeMatrixfARB,#function
		.globl glMultTransposeMatrixdARB ; .type glMultTransposeMatrixdARB,#function
		.globl glMultTransposeMatrixfARB ; .type glMultTransposeMatrixfARB,#function
		.globl glSampleCoverageARB ; .type glSampleCoverageARB,#function
		.globl glCompressedTexImage1DARB ; .type glCompressedTexImage1DARB,#function
		.globl glCompressedTexImage2DARB ; .type glCompressedTexImage2DARB,#function
		.globl glCompressedTexImage3DARB ; .type glCompressedTexImage3DARB,#function
		.globl glCompressedTexSubImage1DARB ; .type glCompressedTexSubImage1DARB,#function
		.globl glCompressedTexSubImage2DARB ; .type glCompressedTexSubImage2DARB,#function
		.globl glCompressedTexSubImage3DARB ; .type glCompressedTexSubImage3DARB,#function
		.globl glGetCompressedTexImageARB ; .type glGetCompressedTexImageARB,#function
		.globl glDisableVertexAttribArrayARB ; .type glDisableVertexAttribArrayARB,#function
		.globl glEnableVertexAttribArrayARB ; .type glEnableVertexAttribArrayARB,#function
		.globl glGetProgramEnvParameterdvARB ; .type glGetProgramEnvParameterdvARB,#function
		.globl glGetProgramEnvParameterfvARB ; .type glGetProgramEnvParameterfvARB,#function
		.globl glGetProgramLocalParameterdvARB ; .type glGetProgramLocalParameterdvARB,#function
		.globl glGetProgramLocalParameterfvARB ; .type glGetProgramLocalParameterfvARB,#function
		.globl glGetProgramStringARB ; .type glGetProgramStringARB,#function
		.globl glGetProgramivARB ; .type glGetProgramivARB,#function
		.globl glGetVertexAttribdvARB ; .type glGetVertexAttribdvARB,#function
		.globl glGetVertexAttribfvARB ; .type glGetVertexAttribfvARB,#function
		.globl glGetVertexAttribivARB ; .type glGetVertexAttribivARB,#function
		.globl glProgramEnvParameter4dARB ; .type glProgramEnvParameter4dARB,#function
		.globl glProgramEnvParameter4dvARB ; .type glProgramEnvParameter4dvARB,#function
		.globl glProgramEnvParameter4fARB ; .type glProgramEnvParameter4fARB,#function
		.globl glProgramEnvParameter4fvARB ; .type glProgramEnvParameter4fvARB,#function
		.globl glProgramLocalParameter4dARB ; .type glProgramLocalParameter4dARB,#function
		.globl glProgramLocalParameter4dvARB ; .type glProgramLocalParameter4dvARB,#function
		.globl glProgramLocalParameter4fARB ; .type glProgramLocalParameter4fARB,#function
		.globl glProgramLocalParameter4fvARB ; .type glProgramLocalParameter4fvARB,#function
		.globl glProgramStringARB ; .type glProgramStringARB,#function
		.globl glVertexAttrib1dARB ; .type glVertexAttrib1dARB,#function
		.globl glVertexAttrib1dvARB ; .type glVertexAttrib1dvARB,#function
		.globl glVertexAttrib1fARB ; .type glVertexAttrib1fARB,#function
		.globl glVertexAttrib1fvARB ; .type glVertexAttrib1fvARB,#function
		.globl glVertexAttrib1sARB ; .type glVertexAttrib1sARB,#function
		.globl glVertexAttrib1svARB ; .type glVertexAttrib1svARB,#function
		.globl glVertexAttrib2dARB ; .type glVertexAttrib2dARB,#function
		.globl glVertexAttrib2dvARB ; .type glVertexAttrib2dvARB,#function
		.globl glVertexAttrib2fARB ; .type glVertexAttrib2fARB,#function
		.globl glVertexAttrib2fvARB ; .type glVertexAttrib2fvARB,#function
		.globl glVertexAttrib2sARB ; .type glVertexAttrib2sARB,#function
		.globl glVertexAttrib2svARB ; .type glVertexAttrib2svARB,#function
		.globl glVertexAttrib3dARB ; .type glVertexAttrib3dARB,#function
		.globl glVertexAttrib3dvARB ; .type glVertexAttrib3dvARB,#function
		.globl glVertexAttrib3fARB ; .type glVertexAttrib3fARB,#function
		.globl glVertexAttrib3fvARB ; .type glVertexAttrib3fvARB,#function
		.globl glVertexAttrib3sARB ; .type glVertexAttrib3sARB,#function
		.globl glVertexAttrib3svARB ; .type glVertexAttrib3svARB,#function
		.globl glVertexAttrib4NbvARB ; .type glVertexAttrib4NbvARB,#function
		.globl glVertexAttrib4NivARB ; .type glVertexAttrib4NivARB,#function
		.globl glVertexAttrib4NsvARB ; .type glVertexAttrib4NsvARB,#function
		.globl glVertexAttrib4NubARB ; .type glVertexAttrib4NubARB,#function
		.globl glVertexAttrib4NubvARB ; .type glVertexAttrib4NubvARB,#function
		.globl glVertexAttrib4NuivARB ; .type glVertexAttrib4NuivARB,#function
		.globl glVertexAttrib4NusvARB ; .type glVertexAttrib4NusvARB,#function
		.globl glVertexAttrib4bvARB ; .type glVertexAttrib4bvARB,#function
		.globl glVertexAttrib4dARB ; .type glVertexAttrib4dARB,#function
		.globl glVertexAttrib4dvARB ; .type glVertexAttrib4dvARB,#function
		.globl glVertexAttrib4fARB ; .type glVertexAttrib4fARB,#function
		.globl glVertexAttrib4fvARB ; .type glVertexAttrib4fvARB,#function
		.globl glVertexAttrib4ivARB ; .type glVertexAttrib4ivARB,#function
		.globl glVertexAttrib4sARB ; .type glVertexAttrib4sARB,#function
		.globl glVertexAttrib4svARB ; .type glVertexAttrib4svARB,#function
		.globl glVertexAttrib4ubvARB ; .type glVertexAttrib4ubvARB,#function
		.globl glVertexAttrib4uivARB ; .type glVertexAttrib4uivARB,#function
		.globl glVertexAttrib4usvARB ; .type glVertexAttrib4usvARB,#function
		.globl glVertexAttribPointerARB ; .type glVertexAttribPointerARB,#function
		.globl glBindBufferARB ; .type glBindBufferARB,#function
		.globl glBufferDataARB ; .type glBufferDataARB,#function
		.globl glBufferSubDataARB ; .type glBufferSubDataARB,#function
		.globl glDeleteBuffersARB ; .type glDeleteBuffersARB,#function
		.globl glGenBuffersARB ; .type glGenBuffersARB,#function
		.globl glGetBufferParameterivARB ; .type glGetBufferParameterivARB,#function
		.globl glGetBufferPointervARB ; .type glGetBufferPointervARB,#function
		.globl glGetBufferSubDataARB ; .type glGetBufferSubDataARB,#function
		.globl glIsBufferARB ; .type glIsBufferARB,#function
		.globl glMapBufferARB ; .type glMapBufferARB,#function
		.globl glUnmapBufferARB ; .type glUnmapBufferARB,#function
		.globl glBeginQueryARB ; .type glBeginQueryARB,#function
		.globl glDeleteQueriesARB ; .type glDeleteQueriesARB,#function
		.globl glEndQueryARB ; .type glEndQueryARB,#function
		.globl glGenQueriesARB ; .type glGenQueriesARB,#function
		.globl glGetQueryObjectivARB ; .type glGetQueryObjectivARB,#function
		.globl glGetQueryObjectuivARB ; .type glGetQueryObjectuivARB,#function
		.globl glGetQueryivARB ; .type glGetQueryivARB,#function
		.globl glIsQueryARB ; .type glIsQueryARB,#function
		.globl glAttachObjectARB ; .type glAttachObjectARB,#function
		.globl glCompileShaderARB ; .type glCompileShaderARB,#function
		.globl glCreateProgramObjectARB ; .type glCreateProgramObjectARB,#function
		.globl glCreateShaderObjectARB ; .type glCreateShaderObjectARB,#function
		.globl glDeleteObjectARB ; .type glDeleteObjectARB,#function
		.globl glDetachObjectARB ; .type glDetachObjectARB,#function
		.globl glGetActiveUniformARB ; .type glGetActiveUniformARB,#function
		.globl glGetAttachedObjectsARB ; .type glGetAttachedObjectsARB,#function
		.globl glGetHandleARB ; .type glGetHandleARB,#function
		.globl glGetInfoLogARB ; .type glGetInfoLogARB,#function
		.globl glGetObjectParameterfvARB ; .type glGetObjectParameterfvARB,#function
		.globl glGetObjectParameterivARB ; .type glGetObjectParameterivARB,#function
		.globl glGetShaderSourceARB ; .type glGetShaderSourceARB,#function
		.globl glGetUniformLocationARB ; .type glGetUniformLocationARB,#function
		.globl glGetUniformfvARB ; .type glGetUniformfvARB,#function
		.globl glGetUniformivARB ; .type glGetUniformivARB,#function
		.globl glLinkProgramARB ; .type glLinkProgramARB,#function
		.globl glShaderSourceARB ; .type glShaderSourceARB,#function
		.globl glUniform1fARB ; .type glUniform1fARB,#function
		.globl glUniform1fvARB ; .type glUniform1fvARB,#function
		.globl glUniform1iARB ; .type glUniform1iARB,#function
		.globl glUniform1ivARB ; .type glUniform1ivARB,#function
		.globl glUniform2fARB ; .type glUniform2fARB,#function
		.globl glUniform2fvARB ; .type glUniform2fvARB,#function
		.globl glUniform2iARB ; .type glUniform2iARB,#function
		.globl glUniform2ivARB ; .type glUniform2ivARB,#function
		.globl glUniform3fARB ; .type glUniform3fARB,#function
		.globl glUniform3fvARB ; .type glUniform3fvARB,#function
		.globl glUniform3iARB ; .type glUniform3iARB,#function
		.globl glUniform3ivARB ; .type glUniform3ivARB,#function
		.globl glUniform4fARB ; .type glUniform4fARB,#function
		.globl glUniform4fvARB ; .type glUniform4fvARB,#function
		.globl glUniform4iARB ; .type glUniform4iARB,#function
		.globl glUniform4ivARB ; .type glUniform4ivARB,#function
		.globl glUniformMatrix2fvARB ; .type glUniformMatrix2fvARB,#function
		.globl glUniformMatrix3fvARB ; .type glUniformMatrix3fvARB,#function
		.globl glUniformMatrix4fvARB ; .type glUniformMatrix4fvARB,#function
		.globl glUseProgramObjectARB ; .type glUseProgramObjectARB,#function
		.globl glValidateProgramARB ; .type glValidateProgramARB,#function
		.globl glBindAttribLocationARB ; .type glBindAttribLocationARB,#function
		.globl glGetActiveAttribARB ; .type glGetActiveAttribARB,#function
		.globl glGetAttribLocationARB ; .type glGetAttribLocationARB,#function
		.globl glDrawBuffersARB ; .type glDrawBuffersARB,#function
		.globl glPolygonOffsetEXT ; .type glPolygonOffsetEXT,#function
		.globl gl_dispatch_stub_562 ; .type gl_dispatch_stub_562,#function
		.globl gl_dispatch_stub_563 ; .type gl_dispatch_stub_563,#function
		.globl gl_dispatch_stub_564 ; .type gl_dispatch_stub_564,#function
		.globl gl_dispatch_stub_565 ; .type gl_dispatch_stub_565,#function
		.globl gl_dispatch_stub_566 ; .type gl_dispatch_stub_566,#function
		.globl gl_dispatch_stub_567 ; .type gl_dispatch_stub_567,#function
		.globl gl_dispatch_stub_568 ; .type gl_dispatch_stub_568,#function
		.globl gl_dispatch_stub_569 ; .type gl_dispatch_stub_569,#function
		.globl glColorPointerEXT ; .type glColorPointerEXT,#function
		.globl glEdgeFlagPointerEXT ; .type glEdgeFlagPointerEXT,#function
		.globl glIndexPointerEXT ; .type glIndexPointerEXT,#function
		.globl glNormalPointerEXT ; .type glNormalPointerEXT,#function
		.globl glTexCoordPointerEXT ; .type glTexCoordPointerEXT,#function
		.globl glVertexPointerEXT ; .type glVertexPointerEXT,#function
		.globl glPointParameterfEXT ; .type glPointParameterfEXT,#function
		.globl glPointParameterfvEXT ; .type glPointParameterfvEXT,#function
		.globl glLockArraysEXT ; .type glLockArraysEXT,#function
		.globl glUnlockArraysEXT ; .type glUnlockArraysEXT,#function
		.globl gl_dispatch_stub_580 ; .type gl_dispatch_stub_580,#function
		.globl gl_dispatch_stub_581 ; .type gl_dispatch_stub_581,#function
		.globl glSecondaryColor3bEXT ; .type glSecondaryColor3bEXT,#function
		.globl glSecondaryColor3bvEXT ; .type glSecondaryColor3bvEXT,#function
		.globl glSecondaryColor3dEXT ; .type glSecondaryColor3dEXT,#function
		.globl glSecondaryColor3dvEXT ; .type glSecondaryColor3dvEXT,#function
		.globl glSecondaryColor3fEXT ; .type glSecondaryColor3fEXT,#function
		.globl glSecondaryColor3fvEXT ; .type glSecondaryColor3fvEXT,#function
		.globl glSecondaryColor3iEXT ; .type glSecondaryColor3iEXT,#function
		.globl glSecondaryColor3ivEXT ; .type glSecondaryColor3ivEXT,#function
		.globl glSecondaryColor3sEXT ; .type glSecondaryColor3sEXT,#function
		.globl glSecondaryColor3svEXT ; .type glSecondaryColor3svEXT,#function
		.globl glSecondaryColor3ubEXT ; .type glSecondaryColor3ubEXT,#function
		.globl glSecondaryColor3ubvEXT ; .type glSecondaryColor3ubvEXT,#function
		.globl glSecondaryColor3uiEXT ; .type glSecondaryColor3uiEXT,#function
		.globl glSecondaryColor3uivEXT ; .type glSecondaryColor3uivEXT,#function
		.globl glSecondaryColor3usEXT ; .type glSecondaryColor3usEXT,#function
		.globl glSecondaryColor3usvEXT ; .type glSecondaryColor3usvEXT,#function
		.globl glSecondaryColorPointerEXT ; .type glSecondaryColorPointerEXT,#function
		.globl glMultiDrawArraysEXT ; .type glMultiDrawArraysEXT,#function
		.globl glMultiDrawElementsEXT ; .type glMultiDrawElementsEXT,#function
		.globl glFogCoordPointerEXT ; .type glFogCoordPointerEXT,#function
		.globl glFogCoorddEXT ; .type glFogCoorddEXT,#function
		.globl glFogCoorddvEXT ; .type glFogCoorddvEXT,#function
		.globl glFogCoordfEXT ; .type glFogCoordfEXT,#function
		.globl glFogCoordfvEXT ; .type glFogCoordfvEXT,#function
		.globl gl_dispatch_stub_606 ; .type gl_dispatch_stub_606,#function
		.globl glBlendFuncSeparateEXT ; .type glBlendFuncSeparateEXT,#function
		.globl glFlushVertexArrayRangeNV ; .type glFlushVertexArrayRangeNV,#function
		.globl glVertexArrayRangeNV ; .type glVertexArrayRangeNV,#function
		.globl glCombinerInputNV ; .type glCombinerInputNV,#function
		.globl glCombinerOutputNV ; .type glCombinerOutputNV,#function
		.globl glCombinerParameterfNV ; .type glCombinerParameterfNV,#function
		.globl glCombinerParameterfvNV ; .type glCombinerParameterfvNV,#function
		.globl glCombinerParameteriNV ; .type glCombinerParameteriNV,#function
		.globl glCombinerParameterivNV ; .type glCombinerParameterivNV,#function
		.globl glFinalCombinerInputNV ; .type glFinalCombinerInputNV,#function
		.globl glGetCombinerInputParameterfvNV ; .type glGetCombinerInputParameterfvNV,#function
		.globl glGetCombinerInputParameterivNV ; .type glGetCombinerInputParameterivNV,#function
		.globl glGetCombinerOutputParameterfvNV ; .type glGetCombinerOutputParameterfvNV,#function
		.globl glGetCombinerOutputParameterivNV ; .type glGetCombinerOutputParameterivNV,#function
		.globl glGetFinalCombinerInputParameterfvNV ; .type glGetFinalCombinerInputParameterfvNV,#function
		.globl glGetFinalCombinerInputParameterivNV ; .type glGetFinalCombinerInputParameterivNV,#function
		.globl glResizeBuffersMESA ; .type glResizeBuffersMESA,#function
		.globl glWindowPos2dMESA ; .type glWindowPos2dMESA,#function
		.globl glWindowPos2dvMESA ; .type glWindowPos2dvMESA,#function
		.globl glWindowPos2fMESA ; .type glWindowPos2fMESA,#function
		.globl glWindowPos2fvMESA ; .type glWindowPos2fvMESA,#function
		.globl glWindowPos2iMESA ; .type glWindowPos2iMESA,#function
		.globl glWindowPos2ivMESA ; .type glWindowPos2ivMESA,#function
		.globl glWindowPos2sMESA ; .type glWindowPos2sMESA,#function
		.globl glWindowPos2svMESA ; .type glWindowPos2svMESA,#function
		.globl glWindowPos3dMESA ; .type glWindowPos3dMESA,#function
		.globl glWindowPos3dvMESA ; .type glWindowPos3dvMESA,#function
		.globl glWindowPos3fMESA ; .type glWindowPos3fMESA,#function
		.globl glWindowPos3fvMESA ; .type glWindowPos3fvMESA,#function
		.globl glWindowPos3iMESA ; .type glWindowPos3iMESA,#function
		.globl glWindowPos3ivMESA ; .type glWindowPos3ivMESA,#function
		.globl glWindowPos3sMESA ; .type glWindowPos3sMESA,#function
		.globl glWindowPos3svMESA ; .type glWindowPos3svMESA,#function
		.globl glWindowPos4dMESA ; .type glWindowPos4dMESA,#function
		.globl glWindowPos4dvMESA ; .type glWindowPos4dvMESA,#function
		.globl glWindowPos4fMESA ; .type glWindowPos4fMESA,#function
		.globl glWindowPos4fvMESA ; .type glWindowPos4fvMESA,#function
		.globl glWindowPos4iMESA ; .type glWindowPos4iMESA,#function
		.globl glWindowPos4ivMESA ; .type glWindowPos4ivMESA,#function
		.globl glWindowPos4sMESA ; .type glWindowPos4sMESA,#function
		.globl glWindowPos4svMESA ; .type glWindowPos4svMESA,#function
		.globl gl_dispatch_stub_648 ; .type gl_dispatch_stub_648,#function
		.globl gl_dispatch_stub_649 ; .type gl_dispatch_stub_649,#function
		.globl gl_dispatch_stub_650 ; .type gl_dispatch_stub_650,#function
		.globl gl_dispatch_stub_651 ; .type gl_dispatch_stub_651,#function
		.globl gl_dispatch_stub_652 ; .type gl_dispatch_stub_652,#function
		.globl gl_dispatch_stub_653 ; .type gl_dispatch_stub_653,#function
		.globl gl_dispatch_stub_654 ; .type gl_dispatch_stub_654,#function
		.globl gl_dispatch_stub_655 ; .type gl_dispatch_stub_655,#function
		.globl gl_dispatch_stub_656 ; .type gl_dispatch_stub_656,#function
		.globl glAreProgramsResidentNV ; .type glAreProgramsResidentNV,#function
		.globl glBindProgramNV ; .type glBindProgramNV,#function
		.globl glDeleteProgramsNV ; .type glDeleteProgramsNV,#function
		.globl glExecuteProgramNV ; .type glExecuteProgramNV,#function
		.globl glGenProgramsNV ; .type glGenProgramsNV,#function
		.globl glGetProgramParameterdvNV ; .type glGetProgramParameterdvNV,#function
		.globl glGetProgramParameterfvNV ; .type glGetProgramParameterfvNV,#function
		.globl glGetProgramStringNV ; .type glGetProgramStringNV,#function
		.globl glGetProgramivNV ; .type glGetProgramivNV,#function
		.globl glGetTrackMatrixivNV ; .type glGetTrackMatrixivNV,#function
		.globl glGetVertexAttribPointervNV ; .type glGetVertexAttribPointervNV,#function
		.globl glGetVertexAttribdvNV ; .type glGetVertexAttribdvNV,#function
		.globl glGetVertexAttribfvNV ; .type glGetVertexAttribfvNV,#function
		.globl glGetVertexAttribivNV ; .type glGetVertexAttribivNV,#function
		.globl glIsProgramNV ; .type glIsProgramNV,#function
		.globl glLoadProgramNV ; .type glLoadProgramNV,#function
		.globl glProgramParameter4dNV ; .type glProgramParameter4dNV,#function
		.globl glProgramParameter4dvNV ; .type glProgramParameter4dvNV,#function
		.globl glProgramParameter4fNV ; .type glProgramParameter4fNV,#function
		.globl glProgramParameter4fvNV ; .type glProgramParameter4fvNV,#function
		.globl glProgramParameters4dvNV ; .type glProgramParameters4dvNV,#function
		.globl glProgramParameters4fvNV ; .type glProgramParameters4fvNV,#function
		.globl glRequestResidentProgramsNV ; .type glRequestResidentProgramsNV,#function
		.globl glTrackMatrixNV ; .type glTrackMatrixNV,#function
		.globl glVertexAttrib1dNV ; .type glVertexAttrib1dNV,#function
		.globl glVertexAttrib1dvNV ; .type glVertexAttrib1dvNV,#function
		.globl glVertexAttrib1fNV ; .type glVertexAttrib1fNV,#function
		.globl glVertexAttrib1fvNV ; .type glVertexAttrib1fvNV,#function
		.globl glVertexAttrib1sNV ; .type glVertexAttrib1sNV,#function
		.globl glVertexAttrib1svNV ; .type glVertexAttrib1svNV,#function
		.globl glVertexAttrib2dNV ; .type glVertexAttrib2dNV,#function
		.globl glVertexAttrib2dvNV ; .type glVertexAttrib2dvNV,#function
		.globl glVertexAttrib2fNV ; .type glVertexAttrib2fNV,#function
		.globl glVertexAttrib2fvNV ; .type glVertexAttrib2fvNV,#function
		.globl glVertexAttrib2sNV ; .type glVertexAttrib2sNV,#function
		.globl glVertexAttrib2svNV ; .type glVertexAttrib2svNV,#function
		.globl glVertexAttrib3dNV ; .type glVertexAttrib3dNV,#function
		.globl glVertexAttrib3dvNV ; .type glVertexAttrib3dvNV,#function
		.globl glVertexAttrib3fNV ; .type glVertexAttrib3fNV,#function
		.globl glVertexAttrib3fvNV ; .type glVertexAttrib3fvNV,#function
		.globl glVertexAttrib3sNV ; .type glVertexAttrib3sNV,#function
		.globl glVertexAttrib3svNV ; .type glVertexAttrib3svNV,#function
		.globl glVertexAttrib4dNV ; .type glVertexAttrib4dNV,#function
		.globl glVertexAttrib4dvNV ; .type glVertexAttrib4dvNV,#function
		.globl glVertexAttrib4fNV ; .type glVertexAttrib4fNV,#function
		.globl glVertexAttrib4fvNV ; .type glVertexAttrib4fvNV,#function
		.globl glVertexAttrib4sNV ; .type glVertexAttrib4sNV,#function
		.globl glVertexAttrib4svNV ; .type glVertexAttrib4svNV,#function
		.globl glVertexAttrib4ubNV ; .type glVertexAttrib4ubNV,#function
		.globl glVertexAttrib4ubvNV ; .type glVertexAttrib4ubvNV,#function
		.globl glVertexAttribPointerNV ; .type glVertexAttribPointerNV,#function
		.globl glVertexAttribs1dvNV ; .type glVertexAttribs1dvNV,#function
		.globl glVertexAttribs1fvNV ; .type glVertexAttribs1fvNV,#function
		.globl glVertexAttribs1svNV ; .type glVertexAttribs1svNV,#function
		.globl glVertexAttribs2dvNV ; .type glVertexAttribs2dvNV,#function
		.globl glVertexAttribs2fvNV ; .type glVertexAttribs2fvNV,#function
		.globl glVertexAttribs2svNV ; .type glVertexAttribs2svNV,#function
		.globl glVertexAttribs3dvNV ; .type glVertexAttribs3dvNV,#function
		.globl glVertexAttribs3fvNV ; .type glVertexAttribs3fvNV,#function
		.globl glVertexAttribs3svNV ; .type glVertexAttribs3svNV,#function
		.globl glVertexAttribs4dvNV ; .type glVertexAttribs4dvNV,#function
		.globl glVertexAttribs4fvNV ; .type glVertexAttribs4fvNV,#function
		.globl glVertexAttribs4svNV ; .type glVertexAttribs4svNV,#function
		.globl glVertexAttribs4ubvNV ; .type glVertexAttribs4ubvNV,#function
		.globl glAlphaFragmentOp1ATI ; .type glAlphaFragmentOp1ATI,#function
		.globl glAlphaFragmentOp2ATI ; .type glAlphaFragmentOp2ATI,#function
		.globl glAlphaFragmentOp3ATI ; .type glAlphaFragmentOp3ATI,#function
		.globl glBeginFragmentShaderATI ; .type glBeginFragmentShaderATI,#function
		.globl glBindFragmentShaderATI ; .type glBindFragmentShaderATI,#function
		.globl glColorFragmentOp1ATI ; .type glColorFragmentOp1ATI,#function
		.globl glColorFragmentOp2ATI ; .type glColorFragmentOp2ATI,#function
		.globl glColorFragmentOp3ATI ; .type glColorFragmentOp3ATI,#function
		.globl glDeleteFragmentShaderATI ; .type glDeleteFragmentShaderATI,#function
		.globl glEndFragmentShaderATI ; .type glEndFragmentShaderATI,#function
		.globl glGenFragmentShadersATI ; .type glGenFragmentShadersATI,#function
		.globl glPassTexCoordATI ; .type glPassTexCoordATI,#function
		.globl glSampleMapATI ; .type glSampleMapATI,#function
		.globl glSetFragmentShaderConstantATI ; .type glSetFragmentShaderConstantATI,#function
		.globl glPointParameteriNV ; .type glPointParameteriNV,#function
		.globl glPointParameterivNV ; .type glPointParameterivNV,#function
		.globl gl_dispatch_stub_737 ; .type gl_dispatch_stub_737,#function
		.globl gl_dispatch_stub_738 ; .type gl_dispatch_stub_738,#function
		.globl gl_dispatch_stub_739 ; .type gl_dispatch_stub_739,#function
		.globl gl_dispatch_stub_740 ; .type gl_dispatch_stub_740,#function
		.globl gl_dispatch_stub_741 ; .type gl_dispatch_stub_741,#function
		.globl glGetProgramNamedParameterdvNV ; .type glGetProgramNamedParameterdvNV,#function
		.globl glGetProgramNamedParameterfvNV ; .type glGetProgramNamedParameterfvNV,#function
		.globl glProgramNamedParameter4dNV ; .type glProgramNamedParameter4dNV,#function
		.globl glProgramNamedParameter4dvNV ; .type glProgramNamedParameter4dvNV,#function
		.globl glProgramNamedParameter4fNV ; .type glProgramNamedParameter4fNV,#function
		.globl glProgramNamedParameter4fvNV ; .type glProgramNamedParameter4fvNV,#function
		.globl gl_dispatch_stub_748 ; .type gl_dispatch_stub_748,#function
		.globl gl_dispatch_stub_749 ; .type gl_dispatch_stub_749,#function
		.globl glBindFramebufferEXT ; .type glBindFramebufferEXT,#function
		.globl glBindRenderbufferEXT ; .type glBindRenderbufferEXT,#function
		.globl glCheckFramebufferStatusEXT ; .type glCheckFramebufferStatusEXT,#function
		.globl glDeleteFramebuffersEXT ; .type glDeleteFramebuffersEXT,#function
		.globl glDeleteRenderbuffersEXT ; .type glDeleteRenderbuffersEXT,#function
		.globl glFramebufferRenderbufferEXT ; .type glFramebufferRenderbufferEXT,#function
		.globl glFramebufferTexture1DEXT ; .type glFramebufferTexture1DEXT,#function
		.globl glFramebufferTexture2DEXT ; .type glFramebufferTexture2DEXT,#function
		.globl glFramebufferTexture3DEXT ; .type glFramebufferTexture3DEXT,#function
		.globl glGenFramebuffersEXT ; .type glGenFramebuffersEXT,#function
		.globl glGenRenderbuffersEXT ; .type glGenRenderbuffersEXT,#function
		.globl glGenerateMipmapEXT ; .type glGenerateMipmapEXT,#function
		.globl glGetFramebufferAttachmentParameterivEXT ; .type glGetFramebufferAttachmentParameterivEXT,#function
		.globl glGetRenderbufferParameterivEXT ; .type glGetRenderbufferParameterivEXT,#function
		.globl glIsFramebufferEXT ; .type glIsFramebufferEXT,#function
		.globl glIsRenderbufferEXT ; .type glIsRenderbufferEXT,#function
		.globl glRenderbufferStorageEXT ; .type glRenderbufferStorageEXT,#function
		.globl gl_dispatch_stub_767 ; .type gl_dispatch_stub_767,#function
		.globl gl_dispatch_stub_768 ; .type gl_dispatch_stub_768,#function
		.globl gl_dispatch_stub_769 ; .type gl_dispatch_stub_769,#function
		.globl gl_dispatch_stub_770 ; .type gl_dispatch_stub_770,#function
		.globl gl_dispatch_stub_771 ; .type gl_dispatch_stub_771,#function
		.globl gl_dispatch_stub_772 ; .type gl_dispatch_stub_772,#function
		.globl _mesa_sparc_glapi_begin ; .type _mesa_sparc_glapi_begin,#function
_mesa_sparc_glapi_begin:

	GL_STUB(glNewList, _gloffset_NewList)
	GL_STUB(glEndList, _gloffset_EndList)
	GL_STUB(glCallList, _gloffset_CallList)
	GL_STUB(glCallLists, _gloffset_CallLists)
	GL_STUB(glDeleteLists, _gloffset_DeleteLists)
	GL_STUB(glGenLists, _gloffset_GenLists)
	GL_STUB(glListBase, _gloffset_ListBase)
	GL_STUB(glBegin, _gloffset_Begin)
	GL_STUB(glBitmap, _gloffset_Bitmap)
	GL_STUB(glColor3b, _gloffset_Color3b)
	GL_STUB(glColor3bv, _gloffset_Color3bv)
	GL_STUB(glColor3d, _gloffset_Color3d)
	GL_STUB(glColor3dv, _gloffset_Color3dv)
	GL_STUB(glColor3f, _gloffset_Color3f)
	GL_STUB(glColor3fv, _gloffset_Color3fv)
	GL_STUB(glColor3i, _gloffset_Color3i)
	GL_STUB(glColor3iv, _gloffset_Color3iv)
	GL_STUB(glColor3s, _gloffset_Color3s)
	GL_STUB(glColor3sv, _gloffset_Color3sv)
	GL_STUB(glColor3ub, _gloffset_Color3ub)
	GL_STUB(glColor3ubv, _gloffset_Color3ubv)
	GL_STUB(glColor3ui, _gloffset_Color3ui)
	GL_STUB(glColor3uiv, _gloffset_Color3uiv)
	GL_STUB(glColor3us, _gloffset_Color3us)
	GL_STUB(glColor3usv, _gloffset_Color3usv)
	GL_STUB(glColor4b, _gloffset_Color4b)
	GL_STUB(glColor4bv, _gloffset_Color4bv)
	GL_STUB(glColor4d, _gloffset_Color4d)
	GL_STUB(glColor4dv, _gloffset_Color4dv)
	GL_STUB(glColor4f, _gloffset_Color4f)
	GL_STUB(glColor4fv, _gloffset_Color4fv)
	GL_STUB(glColor4i, _gloffset_Color4i)
	GL_STUB(glColor4iv, _gloffset_Color4iv)
	GL_STUB(glColor4s, _gloffset_Color4s)
	GL_STUB(glColor4sv, _gloffset_Color4sv)
	GL_STUB(glColor4ub, _gloffset_Color4ub)
	GL_STUB(glColor4ubv, _gloffset_Color4ubv)
	GL_STUB(glColor4ui, _gloffset_Color4ui)
	GL_STUB(glColor4uiv, _gloffset_Color4uiv)
	GL_STUB(glColor4us, _gloffset_Color4us)
	GL_STUB(glColor4usv, _gloffset_Color4usv)
	GL_STUB(glEdgeFlag, _gloffset_EdgeFlag)
	GL_STUB(glEdgeFlagv, _gloffset_EdgeFlagv)
	GL_STUB(glEnd, _gloffset_End)
	GL_STUB(glIndexd, _gloffset_Indexd)
	GL_STUB(glIndexdv, _gloffset_Indexdv)
	GL_STUB(glIndexf, _gloffset_Indexf)
	GL_STUB(glIndexfv, _gloffset_Indexfv)
	GL_STUB(glIndexi, _gloffset_Indexi)
	GL_STUB(glIndexiv, _gloffset_Indexiv)
	GL_STUB(glIndexs, _gloffset_Indexs)
	GL_STUB(glIndexsv, _gloffset_Indexsv)
	GL_STUB(glNormal3b, _gloffset_Normal3b)
	GL_STUB(glNormal3bv, _gloffset_Normal3bv)
	GL_STUB(glNormal3d, _gloffset_Normal3d)
	GL_STUB(glNormal3dv, _gloffset_Normal3dv)
	GL_STUB(glNormal3f, _gloffset_Normal3f)
	GL_STUB(glNormal3fv, _gloffset_Normal3fv)
	GL_STUB(glNormal3i, _gloffset_Normal3i)
	GL_STUB(glNormal3iv, _gloffset_Normal3iv)
	GL_STUB(glNormal3s, _gloffset_Normal3s)
	GL_STUB(glNormal3sv, _gloffset_Normal3sv)
	GL_STUB(glRasterPos2d, _gloffset_RasterPos2d)
	GL_STUB(glRasterPos2dv, _gloffset_RasterPos2dv)
	GL_STUB(glRasterPos2f, _gloffset_RasterPos2f)
	GL_STUB(glRasterPos2fv, _gloffset_RasterPos2fv)
	GL_STUB(glRasterPos2i, _gloffset_RasterPos2i)
	GL_STUB(glRasterPos2iv, _gloffset_RasterPos2iv)
	GL_STUB(glRasterPos2s, _gloffset_RasterPos2s)
	GL_STUB(glRasterPos2sv, _gloffset_RasterPos2sv)
	GL_STUB(glRasterPos3d, _gloffset_RasterPos3d)
	GL_STUB(glRasterPos3dv, _gloffset_RasterPos3dv)
	GL_STUB(glRasterPos3f, _gloffset_RasterPos3f)
	GL_STUB(glRasterPos3fv, _gloffset_RasterPos3fv)
	GL_STUB(glRasterPos3i, _gloffset_RasterPos3i)
	GL_STUB(glRasterPos3iv, _gloffset_RasterPos3iv)
	GL_STUB(glRasterPos3s, _gloffset_RasterPos3s)
	GL_STUB(glRasterPos3sv, _gloffset_RasterPos3sv)
	GL_STUB(glRasterPos4d, _gloffset_RasterPos4d)
	GL_STUB(glRasterPos4dv, _gloffset_RasterPos4dv)
	GL_STUB(glRasterPos4f, _gloffset_RasterPos4f)
	GL_STUB(glRasterPos4fv, _gloffset_RasterPos4fv)
	GL_STUB(glRasterPos4i, _gloffset_RasterPos4i)
	GL_STUB(glRasterPos4iv, _gloffset_RasterPos4iv)
	GL_STUB(glRasterPos4s, _gloffset_RasterPos4s)
	GL_STUB(glRasterPos4sv, _gloffset_RasterPos4sv)
	GL_STUB(glRectd, _gloffset_Rectd)
	GL_STUB(glRectdv, _gloffset_Rectdv)
	GL_STUB(glRectf, _gloffset_Rectf)
	GL_STUB(glRectfv, _gloffset_Rectfv)
	GL_STUB(glRecti, _gloffset_Recti)
	GL_STUB(glRectiv, _gloffset_Rectiv)
	GL_STUB(glRects, _gloffset_Rects)
	GL_STUB(glRectsv, _gloffset_Rectsv)
	GL_STUB(glTexCoord1d, _gloffset_TexCoord1d)
	GL_STUB(glTexCoord1dv, _gloffset_TexCoord1dv)
	GL_STUB(glTexCoord1f, _gloffset_TexCoord1f)
	GL_STUB(glTexCoord1fv, _gloffset_TexCoord1fv)
	GL_STUB(glTexCoord1i, _gloffset_TexCoord1i)
	GL_STUB(glTexCoord1iv, _gloffset_TexCoord1iv)
	GL_STUB(glTexCoord1s, _gloffset_TexCoord1s)
	GL_STUB(glTexCoord1sv, _gloffset_TexCoord1sv)
	GL_STUB(glTexCoord2d, _gloffset_TexCoord2d)
	GL_STUB(glTexCoord2dv, _gloffset_TexCoord2dv)
	GL_STUB(glTexCoord2f, _gloffset_TexCoord2f)
	GL_STUB(glTexCoord2fv, _gloffset_TexCoord2fv)
	GL_STUB(glTexCoord2i, _gloffset_TexCoord2i)
	GL_STUB(glTexCoord2iv, _gloffset_TexCoord2iv)
	GL_STUB(glTexCoord2s, _gloffset_TexCoord2s)
	GL_STUB(glTexCoord2sv, _gloffset_TexCoord2sv)
	GL_STUB(glTexCoord3d, _gloffset_TexCoord3d)
	GL_STUB(glTexCoord3dv, _gloffset_TexCoord3dv)
	GL_STUB(glTexCoord3f, _gloffset_TexCoord3f)
	GL_STUB(glTexCoord3fv, _gloffset_TexCoord3fv)
	GL_STUB(glTexCoord3i, _gloffset_TexCoord3i)
	GL_STUB(glTexCoord3iv, _gloffset_TexCoord3iv)
	GL_STUB(glTexCoord3s, _gloffset_TexCoord3s)
	GL_STUB(glTexCoord3sv, _gloffset_TexCoord3sv)
	GL_STUB(glTexCoord4d, _gloffset_TexCoord4d)
	GL_STUB(glTexCoord4dv, _gloffset_TexCoord4dv)
	GL_STUB(glTexCoord4f, _gloffset_TexCoord4f)
	GL_STUB(glTexCoord4fv, _gloffset_TexCoord4fv)
	GL_STUB(glTexCoord4i, _gloffset_TexCoord4i)
	GL_STUB(glTexCoord4iv, _gloffset_TexCoord4iv)
	GL_STUB(glTexCoord4s, _gloffset_TexCoord4s)
	GL_STUB(glTexCoord4sv, _gloffset_TexCoord4sv)
	GL_STUB(glVertex2d, _gloffset_Vertex2d)
	GL_STUB(glVertex2dv, _gloffset_Vertex2dv)
	GL_STUB(glVertex2f, _gloffset_Vertex2f)
	GL_STUB(glVertex2fv, _gloffset_Vertex2fv)
	GL_STUB(glVertex2i, _gloffset_Vertex2i)
	GL_STUB(glVertex2iv, _gloffset_Vertex2iv)
	GL_STUB(glVertex2s, _gloffset_Vertex2s)
	GL_STUB(glVertex2sv, _gloffset_Vertex2sv)
	GL_STUB(glVertex3d, _gloffset_Vertex3d)
	GL_STUB(glVertex3dv, _gloffset_Vertex3dv)
	GL_STUB(glVertex3f, _gloffset_Vertex3f)
	GL_STUB(glVertex3fv, _gloffset_Vertex3fv)
	GL_STUB(glVertex3i, _gloffset_Vertex3i)
	GL_STUB(glVertex3iv, _gloffset_Vertex3iv)
	GL_STUB(glVertex3s, _gloffset_Vertex3s)
	GL_STUB(glVertex3sv, _gloffset_Vertex3sv)
	GL_STUB(glVertex4d, _gloffset_Vertex4d)
	GL_STUB(glVertex4dv, _gloffset_Vertex4dv)
	GL_STUB(glVertex4f, _gloffset_Vertex4f)
	GL_STUB(glVertex4fv, _gloffset_Vertex4fv)
	GL_STUB(glVertex4i, _gloffset_Vertex4i)
	GL_STUB(glVertex4iv, _gloffset_Vertex4iv)
	GL_STUB(glVertex4s, _gloffset_Vertex4s)
	GL_STUB(glVertex4sv, _gloffset_Vertex4sv)
	GL_STUB(glClipPlane, _gloffset_ClipPlane)
	GL_STUB(glColorMaterial, _gloffset_ColorMaterial)
	GL_STUB(glCullFace, _gloffset_CullFace)
	GL_STUB(glFogf, _gloffset_Fogf)
	GL_STUB(glFogfv, _gloffset_Fogfv)
	GL_STUB(glFogi, _gloffset_Fogi)
	GL_STUB(glFogiv, _gloffset_Fogiv)
	GL_STUB(glFrontFace, _gloffset_FrontFace)
	GL_STUB(glHint, _gloffset_Hint)
	GL_STUB(glLightf, _gloffset_Lightf)
	GL_STUB(glLightfv, _gloffset_Lightfv)
	GL_STUB(glLighti, _gloffset_Lighti)
	GL_STUB(glLightiv, _gloffset_Lightiv)
	GL_STUB(glLightModelf, _gloffset_LightModelf)
	GL_STUB(glLightModelfv, _gloffset_LightModelfv)
	GL_STUB(glLightModeli, _gloffset_LightModeli)
	GL_STUB(glLightModeliv, _gloffset_LightModeliv)
	GL_STUB(glLineStipple, _gloffset_LineStipple)
	GL_STUB(glLineWidth, _gloffset_LineWidth)
	GL_STUB(glMaterialf, _gloffset_Materialf)
	GL_STUB(glMaterialfv, _gloffset_Materialfv)
	GL_STUB(glMateriali, _gloffset_Materiali)
	GL_STUB(glMaterialiv, _gloffset_Materialiv)
	GL_STUB(glPointSize, _gloffset_PointSize)
	GL_STUB(glPolygonMode, _gloffset_PolygonMode)
	GL_STUB(glPolygonStipple, _gloffset_PolygonStipple)
	GL_STUB(glScissor, _gloffset_Scissor)
	GL_STUB(glShadeModel, _gloffset_ShadeModel)
	GL_STUB(glTexParameterf, _gloffset_TexParameterf)
	GL_STUB(glTexParameterfv, _gloffset_TexParameterfv)
	GL_STUB(glTexParameteri, _gloffset_TexParameteri)
	GL_STUB(glTexParameteriv, _gloffset_TexParameteriv)
	GL_STUB(glTexImage1D, _gloffset_TexImage1D)
	GL_STUB(glTexImage2D, _gloffset_TexImage2D)
	GL_STUB(glTexEnvf, _gloffset_TexEnvf)
	GL_STUB(glTexEnvfv, _gloffset_TexEnvfv)
	GL_STUB(glTexEnvi, _gloffset_TexEnvi)
	GL_STUB(glTexEnviv, _gloffset_TexEnviv)
	GL_STUB(glTexGend, _gloffset_TexGend)
	GL_STUB(glTexGendv, _gloffset_TexGendv)
	GL_STUB(glTexGenf, _gloffset_TexGenf)
	GL_STUB(glTexGenfv, _gloffset_TexGenfv)
	GL_STUB(glTexGeni, _gloffset_TexGeni)
	GL_STUB(glTexGeniv, _gloffset_TexGeniv)
	GL_STUB(glFeedbackBuffer, _gloffset_FeedbackBuffer)
	GL_STUB(glSelectBuffer, _gloffset_SelectBuffer)
	GL_STUB(glRenderMode, _gloffset_RenderMode)
	GL_STUB(glInitNames, _gloffset_InitNames)
	GL_STUB(glLoadName, _gloffset_LoadName)
	GL_STUB(glPassThrough, _gloffset_PassThrough)
	GL_STUB(glPopName, _gloffset_PopName)
	GL_STUB(glPushName, _gloffset_PushName)
	GL_STUB(glDrawBuffer, _gloffset_DrawBuffer)
	GL_STUB(glClear, _gloffset_Clear)
	GL_STUB(glClearAccum, _gloffset_ClearAccum)
	GL_STUB(glClearIndex, _gloffset_ClearIndex)
	GL_STUB(glClearColor, _gloffset_ClearColor)
	GL_STUB(glClearStencil, _gloffset_ClearStencil)
	GL_STUB(glClearDepth, _gloffset_ClearDepth)
	GL_STUB(glStencilMask, _gloffset_StencilMask)
	GL_STUB(glColorMask, _gloffset_ColorMask)
	GL_STUB(glDepthMask, _gloffset_DepthMask)
	GL_STUB(glIndexMask, _gloffset_IndexMask)
	GL_STUB(glAccum, _gloffset_Accum)
	GL_STUB(glDisable, _gloffset_Disable)
	GL_STUB(glEnable, _gloffset_Enable)
	GL_STUB(glFinish, _gloffset_Finish)
	GL_STUB(glFlush, _gloffset_Flush)
	GL_STUB(glPopAttrib, _gloffset_PopAttrib)
	GL_STUB(glPushAttrib, _gloffset_PushAttrib)
	GL_STUB(glMap1d, _gloffset_Map1d)
	GL_STUB(glMap1f, _gloffset_Map1f)
	GL_STUB(glMap2d, _gloffset_Map2d)
	GL_STUB(glMap2f, _gloffset_Map2f)
	GL_STUB(glMapGrid1d, _gloffset_MapGrid1d)
	GL_STUB(glMapGrid1f, _gloffset_MapGrid1f)
	GL_STUB(glMapGrid2d, _gloffset_MapGrid2d)
	GL_STUB(glMapGrid2f, _gloffset_MapGrid2f)
	GL_STUB(glEvalCoord1d, _gloffset_EvalCoord1d)
	GL_STUB(glEvalCoord1dv, _gloffset_EvalCoord1dv)
	GL_STUB(glEvalCoord1f, _gloffset_EvalCoord1f)
	GL_STUB(glEvalCoord1fv, _gloffset_EvalCoord1fv)
	GL_STUB(glEvalCoord2d, _gloffset_EvalCoord2d)
	GL_STUB(glEvalCoord2dv, _gloffset_EvalCoord2dv)
	GL_STUB(glEvalCoord2f, _gloffset_EvalCoord2f)
	GL_STUB(glEvalCoord2fv, _gloffset_EvalCoord2fv)
	GL_STUB(glEvalMesh1, _gloffset_EvalMesh1)
	GL_STUB(glEvalPoint1, _gloffset_EvalPoint1)
	GL_STUB(glEvalMesh2, _gloffset_EvalMesh2)
	GL_STUB(glEvalPoint2, _gloffset_EvalPoint2)
	GL_STUB(glAlphaFunc, _gloffset_AlphaFunc)
	GL_STUB(glBlendFunc, _gloffset_BlendFunc)
	GL_STUB(glLogicOp, _gloffset_LogicOp)
	GL_STUB(glStencilFunc, _gloffset_StencilFunc)
	GL_STUB(glStencilOp, _gloffset_StencilOp)
	GL_STUB(glDepthFunc, _gloffset_DepthFunc)
	GL_STUB(glPixelZoom, _gloffset_PixelZoom)
	GL_STUB(glPixelTransferf, _gloffset_PixelTransferf)
	GL_STUB(glPixelTransferi, _gloffset_PixelTransferi)
	GL_STUB(glPixelStoref, _gloffset_PixelStoref)
	GL_STUB(glPixelStorei, _gloffset_PixelStorei)
	GL_STUB(glPixelMapfv, _gloffset_PixelMapfv)
	GL_STUB(glPixelMapuiv, _gloffset_PixelMapuiv)
	GL_STUB(glPixelMapusv, _gloffset_PixelMapusv)
	GL_STUB(glReadBuffer, _gloffset_ReadBuffer)
	GL_STUB(glCopyPixels, _gloffset_CopyPixels)
	GL_STUB(glReadPixels, _gloffset_ReadPixels)
	GL_STUB(glDrawPixels, _gloffset_DrawPixels)
	GL_STUB(glGetBooleanv, _gloffset_GetBooleanv)
	GL_STUB(glGetClipPlane, _gloffset_GetClipPlane)
	GL_STUB(glGetDoublev, _gloffset_GetDoublev)
	GL_STUB(glGetError, _gloffset_GetError)
	GL_STUB(glGetFloatv, _gloffset_GetFloatv)
	GL_STUB(glGetIntegerv, _gloffset_GetIntegerv)
	GL_STUB(glGetLightfv, _gloffset_GetLightfv)
	GL_STUB(glGetLightiv, _gloffset_GetLightiv)
	GL_STUB(glGetMapdv, _gloffset_GetMapdv)
	GL_STUB(glGetMapfv, _gloffset_GetMapfv)
	GL_STUB(glGetMapiv, _gloffset_GetMapiv)
	GL_STUB(glGetMaterialfv, _gloffset_GetMaterialfv)
	GL_STUB(glGetMaterialiv, _gloffset_GetMaterialiv)
	GL_STUB(glGetPixelMapfv, _gloffset_GetPixelMapfv)
	GL_STUB(glGetPixelMapuiv, _gloffset_GetPixelMapuiv)
	GL_STUB(glGetPixelMapusv, _gloffset_GetPixelMapusv)
	GL_STUB(glGetPolygonStipple, _gloffset_GetPolygonStipple)
	GL_STUB(glGetString, _gloffset_GetString)
	GL_STUB(glGetTexEnvfv, _gloffset_GetTexEnvfv)
	GL_STUB(glGetTexEnviv, _gloffset_GetTexEnviv)
	GL_STUB(glGetTexGendv, _gloffset_GetTexGendv)
	GL_STUB(glGetTexGenfv, _gloffset_GetTexGenfv)
	GL_STUB(glGetTexGeniv, _gloffset_GetTexGeniv)
	GL_STUB(glGetTexImage, _gloffset_GetTexImage)
	GL_STUB(glGetTexParameterfv, _gloffset_GetTexParameterfv)
	GL_STUB(glGetTexParameteriv, _gloffset_GetTexParameteriv)
	GL_STUB(glGetTexLevelParameterfv, _gloffset_GetTexLevelParameterfv)
	GL_STUB(glGetTexLevelParameteriv, _gloffset_GetTexLevelParameteriv)
	GL_STUB(glIsEnabled, _gloffset_IsEnabled)
	GL_STUB(glIsList, _gloffset_IsList)
	GL_STUB(glDepthRange, _gloffset_DepthRange)
	GL_STUB(glFrustum, _gloffset_Frustum)
	GL_STUB(glLoadIdentity, _gloffset_LoadIdentity)
	GL_STUB(glLoadMatrixf, _gloffset_LoadMatrixf)
	GL_STUB(glLoadMatrixd, _gloffset_LoadMatrixd)
	GL_STUB(glMatrixMode, _gloffset_MatrixMode)
	GL_STUB(glMultMatrixf, _gloffset_MultMatrixf)
	GL_STUB(glMultMatrixd, _gloffset_MultMatrixd)
	GL_STUB(glOrtho, _gloffset_Ortho)
	GL_STUB(glPopMatrix, _gloffset_PopMatrix)
	GL_STUB(glPushMatrix, _gloffset_PushMatrix)
	GL_STUB(glRotated, _gloffset_Rotated)
	GL_STUB(glRotatef, _gloffset_Rotatef)
	GL_STUB(glScaled, _gloffset_Scaled)
	GL_STUB(glScalef, _gloffset_Scalef)
	GL_STUB(glTranslated, _gloffset_Translated)
	GL_STUB(glTranslatef, _gloffset_Translatef)
	GL_STUB(glViewport, _gloffset_Viewport)
	GL_STUB(glArrayElement, _gloffset_ArrayElement)
	GL_STUB(glBindTexture, _gloffset_BindTexture)
	GL_STUB(glColorPointer, _gloffset_ColorPointer)
	GL_STUB(glDisableClientState, _gloffset_DisableClientState)
	GL_STUB(glDrawArrays, _gloffset_DrawArrays)
	GL_STUB(glDrawElements, _gloffset_DrawElements)
	GL_STUB(glEdgeFlagPointer, _gloffset_EdgeFlagPointer)
	GL_STUB(glEnableClientState, _gloffset_EnableClientState)
	GL_STUB(glIndexPointer, _gloffset_IndexPointer)
	GL_STUB(glIndexub, _gloffset_Indexub)
	GL_STUB(glIndexubv, _gloffset_Indexubv)
	GL_STUB(glInterleavedArrays, _gloffset_InterleavedArrays)
	GL_STUB(glNormalPointer, _gloffset_NormalPointer)
	GL_STUB(glPolygonOffset, _gloffset_PolygonOffset)
	GL_STUB(glTexCoordPointer, _gloffset_TexCoordPointer)
	GL_STUB(glVertexPointer, _gloffset_VertexPointer)
	GL_STUB(glAreTexturesResident, _gloffset_AreTexturesResident)
	GL_STUB(glCopyTexImage1D, _gloffset_CopyTexImage1D)
	GL_STUB(glCopyTexImage2D, _gloffset_CopyTexImage2D)
	GL_STUB(glCopyTexSubImage1D, _gloffset_CopyTexSubImage1D)
	GL_STUB(glCopyTexSubImage2D, _gloffset_CopyTexSubImage2D)
	GL_STUB(glDeleteTextures, _gloffset_DeleteTextures)
	GL_STUB(glGenTextures, _gloffset_GenTextures)
	GL_STUB(glGetPointerv, _gloffset_GetPointerv)
	GL_STUB(glIsTexture, _gloffset_IsTexture)
	GL_STUB(glPrioritizeTextures, _gloffset_PrioritizeTextures)
	GL_STUB(glTexSubImage1D, _gloffset_TexSubImage1D)
	GL_STUB(glTexSubImage2D, _gloffset_TexSubImage2D)
	GL_STUB(glPopClientAttrib, _gloffset_PopClientAttrib)
	GL_STUB(glPushClientAttrib, _gloffset_PushClientAttrib)
	GL_STUB(glBlendColor, _gloffset_BlendColor)
	GL_STUB(glBlendEquation, _gloffset_BlendEquation)
	GL_STUB(glDrawRangeElements, _gloffset_DrawRangeElements)
	GL_STUB(glColorTable, _gloffset_ColorTable)
	GL_STUB(glColorTableParameterfv, _gloffset_ColorTableParameterfv)
	GL_STUB(glColorTableParameteriv, _gloffset_ColorTableParameteriv)
	GL_STUB(glCopyColorTable, _gloffset_CopyColorTable)
	GL_STUB(glGetColorTable, _gloffset_GetColorTable)
	GL_STUB(glGetColorTableParameterfv, _gloffset_GetColorTableParameterfv)
	GL_STUB(glGetColorTableParameteriv, _gloffset_GetColorTableParameteriv)
	GL_STUB(glColorSubTable, _gloffset_ColorSubTable)
	GL_STUB(glCopyColorSubTable, _gloffset_CopyColorSubTable)
	GL_STUB(glConvolutionFilter1D, _gloffset_ConvolutionFilter1D)
	GL_STUB(glConvolutionFilter2D, _gloffset_ConvolutionFilter2D)
	GL_STUB(glConvolutionParameterf, _gloffset_ConvolutionParameterf)
	GL_STUB(glConvolutionParameterfv, _gloffset_ConvolutionParameterfv)
	GL_STUB(glConvolutionParameteri, _gloffset_ConvolutionParameteri)
	GL_STUB(glConvolutionParameteriv, _gloffset_ConvolutionParameteriv)
	GL_STUB(glCopyConvolutionFilter1D, _gloffset_CopyConvolutionFilter1D)
	GL_STUB(glCopyConvolutionFilter2D, _gloffset_CopyConvolutionFilter2D)
	GL_STUB(glGetConvolutionFilter, _gloffset_GetConvolutionFilter)
	GL_STUB(glGetConvolutionParameterfv, _gloffset_GetConvolutionParameterfv)
	GL_STUB(glGetConvolutionParameteriv, _gloffset_GetConvolutionParameteriv)
	GL_STUB(glGetSeparableFilter, _gloffset_GetSeparableFilter)
	GL_STUB(glSeparableFilter2D, _gloffset_SeparableFilter2D)
	GL_STUB(glGetHistogram, _gloffset_GetHistogram)
	GL_STUB(glGetHistogramParameterfv, _gloffset_GetHistogramParameterfv)
	GL_STUB(glGetHistogramParameteriv, _gloffset_GetHistogramParameteriv)
	GL_STUB(glGetMinmax, _gloffset_GetMinmax)
	GL_STUB(glGetMinmaxParameterfv, _gloffset_GetMinmaxParameterfv)
	GL_STUB(glGetMinmaxParameteriv, _gloffset_GetMinmaxParameteriv)
	GL_STUB(glHistogram, _gloffset_Histogram)
	GL_STUB(glMinmax, _gloffset_Minmax)
	GL_STUB(glResetHistogram, _gloffset_ResetHistogram)
	GL_STUB(glResetMinmax, _gloffset_ResetMinmax)
	GL_STUB(glTexImage3D, _gloffset_TexImage3D)
	GL_STUB(glTexSubImage3D, _gloffset_TexSubImage3D)
	GL_STUB(glCopyTexSubImage3D, _gloffset_CopyTexSubImage3D)
	GL_STUB(glActiveTextureARB, _gloffset_ActiveTextureARB)
	GL_STUB(glClientActiveTextureARB, _gloffset_ClientActiveTextureARB)
	GL_STUB(glMultiTexCoord1dARB, _gloffset_MultiTexCoord1dARB)
	GL_STUB(glMultiTexCoord1dvARB, _gloffset_MultiTexCoord1dvARB)
	GL_STUB(glMultiTexCoord1fARB, _gloffset_MultiTexCoord1fARB)
	GL_STUB(glMultiTexCoord1fvARB, _gloffset_MultiTexCoord1fvARB)
	GL_STUB(glMultiTexCoord1iARB, _gloffset_MultiTexCoord1iARB)
	GL_STUB(glMultiTexCoord1ivARB, _gloffset_MultiTexCoord1ivARB)
	GL_STUB(glMultiTexCoord1sARB, _gloffset_MultiTexCoord1sARB)
	GL_STUB(glMultiTexCoord1svARB, _gloffset_MultiTexCoord1svARB)
	GL_STUB(glMultiTexCoord2dARB, _gloffset_MultiTexCoord2dARB)
	GL_STUB(glMultiTexCoord2dvARB, _gloffset_MultiTexCoord2dvARB)
	GL_STUB(glMultiTexCoord2fARB, _gloffset_MultiTexCoord2fARB)
	GL_STUB(glMultiTexCoord2fvARB, _gloffset_MultiTexCoord2fvARB)
	GL_STUB(glMultiTexCoord2iARB, _gloffset_MultiTexCoord2iARB)
	GL_STUB(glMultiTexCoord2ivARB, _gloffset_MultiTexCoord2ivARB)
	GL_STUB(glMultiTexCoord2sARB, _gloffset_MultiTexCoord2sARB)
	GL_STUB(glMultiTexCoord2svARB, _gloffset_MultiTexCoord2svARB)
	GL_STUB(glMultiTexCoord3dARB, _gloffset_MultiTexCoord3dARB)
	GL_STUB(glMultiTexCoord3dvARB, _gloffset_MultiTexCoord3dvARB)
	GL_STUB(glMultiTexCoord3fARB, _gloffset_MultiTexCoord3fARB)
	GL_STUB(glMultiTexCoord3fvARB, _gloffset_MultiTexCoord3fvARB)
	GL_STUB(glMultiTexCoord3iARB, _gloffset_MultiTexCoord3iARB)
	GL_STUB(glMultiTexCoord3ivARB, _gloffset_MultiTexCoord3ivARB)
	GL_STUB(glMultiTexCoord3sARB, _gloffset_MultiTexCoord3sARB)
	GL_STUB(glMultiTexCoord3svARB, _gloffset_MultiTexCoord3svARB)
	GL_STUB(glMultiTexCoord4dARB, _gloffset_MultiTexCoord4dARB)
	GL_STUB(glMultiTexCoord4dvARB, _gloffset_MultiTexCoord4dvARB)
	GL_STUB(glMultiTexCoord4fARB, _gloffset_MultiTexCoord4fARB)
	GL_STUB(glMultiTexCoord4fvARB, _gloffset_MultiTexCoord4fvARB)
	GL_STUB(glMultiTexCoord4iARB, _gloffset_MultiTexCoord4iARB)
	GL_STUB(glMultiTexCoord4ivARB, _gloffset_MultiTexCoord4ivARB)
	GL_STUB(glMultiTexCoord4sARB, _gloffset_MultiTexCoord4sARB)
	GL_STUB(glMultiTexCoord4svARB, _gloffset_MultiTexCoord4svARB)
	GL_STUB(glAttachShader, _gloffset_AttachShader)
	GL_STUB(glCreateProgram, _gloffset_CreateProgram)
	GL_STUB(glCreateShader, _gloffset_CreateShader)
	GL_STUB(glDeleteProgram, _gloffset_DeleteProgram)
	GL_STUB(glDeleteShader, _gloffset_DeleteShader)
	GL_STUB(glDetachShader, _gloffset_DetachShader)
	GL_STUB(glGetAttachedShaders, _gloffset_GetAttachedShaders)
	GL_STUB(glGetProgramInfoLog, _gloffset_GetProgramInfoLog)
	GL_STUB(glGetProgramiv, _gloffset_GetProgramiv)
	GL_STUB(glGetShaderInfoLog, _gloffset_GetShaderInfoLog)
	GL_STUB(glGetShaderiv, _gloffset_GetShaderiv)
	GL_STUB(glIsProgram, _gloffset_IsProgram)
	GL_STUB(glIsShader, _gloffset_IsShader)
	GL_STUB(glStencilFuncSeparate, _gloffset_StencilFuncSeparate)
	GL_STUB(glStencilMaskSeparate, _gloffset_StencilMaskSeparate)
	GL_STUB(glStencilOpSeparate, _gloffset_StencilOpSeparate)
	GL_STUB(glUniformMatrix2x3fv, _gloffset_UniformMatrix2x3fv)
	GL_STUB(glUniformMatrix2x4fv, _gloffset_UniformMatrix2x4fv)
	GL_STUB(glUniformMatrix3x2fv, _gloffset_UniformMatrix3x2fv)
	GL_STUB(glUniformMatrix3x4fv, _gloffset_UniformMatrix3x4fv)
	GL_STUB(glUniformMatrix4x2fv, _gloffset_UniformMatrix4x2fv)
	GL_STUB(glUniformMatrix4x3fv, _gloffset_UniformMatrix4x3fv)
	GL_STUB(glLoadTransposeMatrixdARB, _gloffset_LoadTransposeMatrixdARB)
	GL_STUB(glLoadTransposeMatrixfARB, _gloffset_LoadTransposeMatrixfARB)
	GL_STUB(glMultTransposeMatrixdARB, _gloffset_MultTransposeMatrixdARB)
	GL_STUB(glMultTransposeMatrixfARB, _gloffset_MultTransposeMatrixfARB)
	GL_STUB(glSampleCoverageARB, _gloffset_SampleCoverageARB)
	GL_STUB(glCompressedTexImage1DARB, _gloffset_CompressedTexImage1DARB)
	GL_STUB(glCompressedTexImage2DARB, _gloffset_CompressedTexImage2DARB)
	GL_STUB(glCompressedTexImage3DARB, _gloffset_CompressedTexImage3DARB)
	GL_STUB(glCompressedTexSubImage1DARB, _gloffset_CompressedTexSubImage1DARB)
	GL_STUB(glCompressedTexSubImage2DARB, _gloffset_CompressedTexSubImage2DARB)
	GL_STUB(glCompressedTexSubImage3DARB, _gloffset_CompressedTexSubImage3DARB)
	GL_STUB(glGetCompressedTexImageARB, _gloffset_GetCompressedTexImageARB)
	GL_STUB(glDisableVertexAttribArrayARB, _gloffset_DisableVertexAttribArrayARB)
	GL_STUB(glEnableVertexAttribArrayARB, _gloffset_EnableVertexAttribArrayARB)
	GL_STUB(glGetProgramEnvParameterdvARB, _gloffset_GetProgramEnvParameterdvARB)
	GL_STUB(glGetProgramEnvParameterfvARB, _gloffset_GetProgramEnvParameterfvARB)
	GL_STUB(glGetProgramLocalParameterdvARB, _gloffset_GetProgramLocalParameterdvARB)
	GL_STUB(glGetProgramLocalParameterfvARB, _gloffset_GetProgramLocalParameterfvARB)
	GL_STUB(glGetProgramStringARB, _gloffset_GetProgramStringARB)
	GL_STUB(glGetProgramivARB, _gloffset_GetProgramivARB)
	GL_STUB(glGetVertexAttribdvARB, _gloffset_GetVertexAttribdvARB)
	GL_STUB(glGetVertexAttribfvARB, _gloffset_GetVertexAttribfvARB)
	GL_STUB(glGetVertexAttribivARB, _gloffset_GetVertexAttribivARB)
	GL_STUB(glProgramEnvParameter4dARB, _gloffset_ProgramEnvParameter4dARB)
	GL_STUB(glProgramEnvParameter4dvARB, _gloffset_ProgramEnvParameter4dvARB)
	GL_STUB(glProgramEnvParameter4fARB, _gloffset_ProgramEnvParameter4fARB)
	GL_STUB(glProgramEnvParameter4fvARB, _gloffset_ProgramEnvParameter4fvARB)
	GL_STUB(glProgramLocalParameter4dARB, _gloffset_ProgramLocalParameter4dARB)
	GL_STUB(glProgramLocalParameter4dvARB, _gloffset_ProgramLocalParameter4dvARB)
	GL_STUB(glProgramLocalParameter4fARB, _gloffset_ProgramLocalParameter4fARB)
	GL_STUB(glProgramLocalParameter4fvARB, _gloffset_ProgramLocalParameter4fvARB)
	GL_STUB(glProgramStringARB, _gloffset_ProgramStringARB)
	GL_STUB(glVertexAttrib1dARB, _gloffset_VertexAttrib1dARB)
	GL_STUB(glVertexAttrib1dvARB, _gloffset_VertexAttrib1dvARB)
	GL_STUB(glVertexAttrib1fARB, _gloffset_VertexAttrib1fARB)
	GL_STUB(glVertexAttrib1fvARB, _gloffset_VertexAttrib1fvARB)
	GL_STUB(glVertexAttrib1sARB, _gloffset_VertexAttrib1sARB)
	GL_STUB(glVertexAttrib1svARB, _gloffset_VertexAttrib1svARB)
	GL_STUB(glVertexAttrib2dARB, _gloffset_VertexAttrib2dARB)
	GL_STUB(glVertexAttrib2dvARB, _gloffset_VertexAttrib2dvARB)
	GL_STUB(glVertexAttrib2fARB, _gloffset_VertexAttrib2fARB)
	GL_STUB(glVertexAttrib2fvARB, _gloffset_VertexAttrib2fvARB)
	GL_STUB(glVertexAttrib2sARB, _gloffset_VertexAttrib2sARB)
	GL_STUB(glVertexAttrib2svARB, _gloffset_VertexAttrib2svARB)
	GL_STUB(glVertexAttrib3dARB, _gloffset_VertexAttrib3dARB)
	GL_STUB(glVertexAttrib3dvARB, _gloffset_VertexAttrib3dvARB)
	GL_STUB(glVertexAttrib3fARB, _gloffset_VertexAttrib3fARB)
	GL_STUB(glVertexAttrib3fvARB, _gloffset_VertexAttrib3fvARB)
	GL_STUB(glVertexAttrib3sARB, _gloffset_VertexAttrib3sARB)
	GL_STUB(glVertexAttrib3svARB, _gloffset_VertexAttrib3svARB)
	GL_STUB(glVertexAttrib4NbvARB, _gloffset_VertexAttrib4NbvARB)
	GL_STUB(glVertexAttrib4NivARB, _gloffset_VertexAttrib4NivARB)
	GL_STUB(glVertexAttrib4NsvARB, _gloffset_VertexAttrib4NsvARB)
	GL_STUB(glVertexAttrib4NubARB, _gloffset_VertexAttrib4NubARB)
	GL_STUB(glVertexAttrib4NubvARB, _gloffset_VertexAttrib4NubvARB)
	GL_STUB(glVertexAttrib4NuivARB, _gloffset_VertexAttrib4NuivARB)
	GL_STUB(glVertexAttrib4NusvARB, _gloffset_VertexAttrib4NusvARB)
	GL_STUB(glVertexAttrib4bvARB, _gloffset_VertexAttrib4bvARB)
	GL_STUB(glVertexAttrib4dARB, _gloffset_VertexAttrib4dARB)
	GL_STUB(glVertexAttrib4dvARB, _gloffset_VertexAttrib4dvARB)
	GL_STUB(glVertexAttrib4fARB, _gloffset_VertexAttrib4fARB)
	GL_STUB(glVertexAttrib4fvARB, _gloffset_VertexAttrib4fvARB)
	GL_STUB(glVertexAttrib4ivARB, _gloffset_VertexAttrib4ivARB)
	GL_STUB(glVertexAttrib4sARB, _gloffset_VertexAttrib4sARB)
	GL_STUB(glVertexAttrib4svARB, _gloffset_VertexAttrib4svARB)
	GL_STUB(glVertexAttrib4ubvARB, _gloffset_VertexAttrib4ubvARB)
	GL_STUB(glVertexAttrib4uivARB, _gloffset_VertexAttrib4uivARB)
	GL_STUB(glVertexAttrib4usvARB, _gloffset_VertexAttrib4usvARB)
	GL_STUB(glVertexAttribPointerARB, _gloffset_VertexAttribPointerARB)
	GL_STUB(glBindBufferARB, _gloffset_BindBufferARB)
	GL_STUB(glBufferDataARB, _gloffset_BufferDataARB)
	GL_STUB(glBufferSubDataARB, _gloffset_BufferSubDataARB)
	GL_STUB(glDeleteBuffersARB, _gloffset_DeleteBuffersARB)
	GL_STUB(glGenBuffersARB, _gloffset_GenBuffersARB)
	GL_STUB(glGetBufferParameterivARB, _gloffset_GetBufferParameterivARB)
	GL_STUB(glGetBufferPointervARB, _gloffset_GetBufferPointervARB)
	GL_STUB(glGetBufferSubDataARB, _gloffset_GetBufferSubDataARB)
	GL_STUB(glIsBufferARB, _gloffset_IsBufferARB)
	GL_STUB(glMapBufferARB, _gloffset_MapBufferARB)
	GL_STUB(glUnmapBufferARB, _gloffset_UnmapBufferARB)
	GL_STUB(glBeginQueryARB, _gloffset_BeginQueryARB)
	GL_STUB(glDeleteQueriesARB, _gloffset_DeleteQueriesARB)
	GL_STUB(glEndQueryARB, _gloffset_EndQueryARB)
	GL_STUB(glGenQueriesARB, _gloffset_GenQueriesARB)
	GL_STUB(glGetQueryObjectivARB, _gloffset_GetQueryObjectivARB)
	GL_STUB(glGetQueryObjectuivARB, _gloffset_GetQueryObjectuivARB)
	GL_STUB(glGetQueryivARB, _gloffset_GetQueryivARB)
	GL_STUB(glIsQueryARB, _gloffset_IsQueryARB)
	GL_STUB(glAttachObjectARB, _gloffset_AttachObjectARB)
	GL_STUB(glCompileShaderARB, _gloffset_CompileShaderARB)
	GL_STUB(glCreateProgramObjectARB, _gloffset_CreateProgramObjectARB)
	GL_STUB(glCreateShaderObjectARB, _gloffset_CreateShaderObjectARB)
	GL_STUB(glDeleteObjectARB, _gloffset_DeleteObjectARB)
	GL_STUB(glDetachObjectARB, _gloffset_DetachObjectARB)
	GL_STUB(glGetActiveUniformARB, _gloffset_GetActiveUniformARB)
	GL_STUB(glGetAttachedObjectsARB, _gloffset_GetAttachedObjectsARB)
	GL_STUB(glGetHandleARB, _gloffset_GetHandleARB)
	GL_STUB(glGetInfoLogARB, _gloffset_GetInfoLogARB)
	GL_STUB(glGetObjectParameterfvARB, _gloffset_GetObjectParameterfvARB)
	GL_STUB(glGetObjectParameterivARB, _gloffset_GetObjectParameterivARB)
	GL_STUB(glGetShaderSourceARB, _gloffset_GetShaderSourceARB)
	GL_STUB(glGetUniformLocationARB, _gloffset_GetUniformLocationARB)
	GL_STUB(glGetUniformfvARB, _gloffset_GetUniformfvARB)
	GL_STUB(glGetUniformivARB, _gloffset_GetUniformivARB)
	GL_STUB(glLinkProgramARB, _gloffset_LinkProgramARB)
	GL_STUB(glShaderSourceARB, _gloffset_ShaderSourceARB)
	GL_STUB(glUniform1fARB, _gloffset_Uniform1fARB)
	GL_STUB(glUniform1fvARB, _gloffset_Uniform1fvARB)
	GL_STUB(glUniform1iARB, _gloffset_Uniform1iARB)
	GL_STUB(glUniform1ivARB, _gloffset_Uniform1ivARB)
	GL_STUB(glUniform2fARB, _gloffset_Uniform2fARB)
	GL_STUB(glUniform2fvARB, _gloffset_Uniform2fvARB)
	GL_STUB(glUniform2iARB, _gloffset_Uniform2iARB)
	GL_STUB(glUniform2ivARB, _gloffset_Uniform2ivARB)
	GL_STUB(glUniform3fARB, _gloffset_Uniform3fARB)
	GL_STUB(glUniform3fvARB, _gloffset_Uniform3fvARB)
	GL_STUB(glUniform3iARB, _gloffset_Uniform3iARB)
	GL_STUB(glUniform3ivARB, _gloffset_Uniform3ivARB)
	GL_STUB(glUniform4fARB, _gloffset_Uniform4fARB)
	GL_STUB(glUniform4fvARB, _gloffset_Uniform4fvARB)
	GL_STUB(glUniform4iARB, _gloffset_Uniform4iARB)
	GL_STUB(glUniform4ivARB, _gloffset_Uniform4ivARB)
	GL_STUB(glUniformMatrix2fvARB, _gloffset_UniformMatrix2fvARB)
	GL_STUB(glUniformMatrix3fvARB, _gloffset_UniformMatrix3fvARB)
	GL_STUB(glUniformMatrix4fvARB, _gloffset_UniformMatrix4fvARB)
	GL_STUB(glUseProgramObjectARB, _gloffset_UseProgramObjectARB)
	GL_STUB(glValidateProgramARB, _gloffset_ValidateProgramARB)
	GL_STUB(glBindAttribLocationARB, _gloffset_BindAttribLocationARB)
	GL_STUB(glGetActiveAttribARB, _gloffset_GetActiveAttribARB)
	GL_STUB(glGetAttribLocationARB, _gloffset_GetAttribLocationARB)
	GL_STUB(glDrawBuffersARB, _gloffset_DrawBuffersARB)
	GL_STUB(glPolygonOffsetEXT, _gloffset_PolygonOffsetEXT)
	GL_STUB(gl_dispatch_stub_562, _gloffset__dispatch_stub_562)
	GL_STUB(gl_dispatch_stub_563, _gloffset__dispatch_stub_563)
	GL_STUB(gl_dispatch_stub_564, _gloffset__dispatch_stub_564)
	GL_STUB(gl_dispatch_stub_565, _gloffset__dispatch_stub_565)
	GL_STUB(gl_dispatch_stub_566, _gloffset__dispatch_stub_566)
	GL_STUB(gl_dispatch_stub_567, _gloffset__dispatch_stub_567)
	GL_STUB(gl_dispatch_stub_568, _gloffset__dispatch_stub_568)
	GL_STUB(gl_dispatch_stub_569, _gloffset__dispatch_stub_569)
	GL_STUB(glColorPointerEXT, _gloffset_ColorPointerEXT)
	GL_STUB(glEdgeFlagPointerEXT, _gloffset_EdgeFlagPointerEXT)
	GL_STUB(glIndexPointerEXT, _gloffset_IndexPointerEXT)
	GL_STUB(glNormalPointerEXT, _gloffset_NormalPointerEXT)
	GL_STUB(glTexCoordPointerEXT, _gloffset_TexCoordPointerEXT)
	GL_STUB(glVertexPointerEXT, _gloffset_VertexPointerEXT)
	GL_STUB(glPointParameterfEXT, _gloffset_PointParameterfEXT)
	GL_STUB(glPointParameterfvEXT, _gloffset_PointParameterfvEXT)
	GL_STUB(glLockArraysEXT, _gloffset_LockArraysEXT)
	GL_STUB(glUnlockArraysEXT, _gloffset_UnlockArraysEXT)
	GL_STUB(gl_dispatch_stub_580, _gloffset__dispatch_stub_580)
	GL_STUB(gl_dispatch_stub_581, _gloffset__dispatch_stub_581)
	GL_STUB(glSecondaryColor3bEXT, _gloffset_SecondaryColor3bEXT)
	GL_STUB(glSecondaryColor3bvEXT, _gloffset_SecondaryColor3bvEXT)
	GL_STUB(glSecondaryColor3dEXT, _gloffset_SecondaryColor3dEXT)
	GL_STUB(glSecondaryColor3dvEXT, _gloffset_SecondaryColor3dvEXT)
	GL_STUB(glSecondaryColor3fEXT, _gloffset_SecondaryColor3fEXT)
	GL_STUB(glSecondaryColor3fvEXT, _gloffset_SecondaryColor3fvEXT)
	GL_STUB(glSecondaryColor3iEXT, _gloffset_SecondaryColor3iEXT)
	GL_STUB(glSecondaryColor3ivEXT, _gloffset_SecondaryColor3ivEXT)
	GL_STUB(glSecondaryColor3sEXT, _gloffset_SecondaryColor3sEXT)
	GL_STUB(glSecondaryColor3svEXT, _gloffset_SecondaryColor3svEXT)
	GL_STUB(glSecondaryColor3ubEXT, _gloffset_SecondaryColor3ubEXT)
	GL_STUB(glSecondaryColor3ubvEXT, _gloffset_SecondaryColor3ubvEXT)
	GL_STUB(glSecondaryColor3uiEXT, _gloffset_SecondaryColor3uiEXT)
	GL_STUB(glSecondaryColor3uivEXT, _gloffset_SecondaryColor3uivEXT)
	GL_STUB(glSecondaryColor3usEXT, _gloffset_SecondaryColor3usEXT)
	GL_STUB(glSecondaryColor3usvEXT, _gloffset_SecondaryColor3usvEXT)
	GL_STUB(glSecondaryColorPointerEXT, _gloffset_SecondaryColorPointerEXT)
	GL_STUB(glMultiDrawArraysEXT, _gloffset_MultiDrawArraysEXT)
	GL_STUB(glMultiDrawElementsEXT, _gloffset_MultiDrawElementsEXT)
	GL_STUB(glFogCoordPointerEXT, _gloffset_FogCoordPointerEXT)
	GL_STUB(glFogCoorddEXT, _gloffset_FogCoorddEXT)
	GL_STUB(glFogCoorddvEXT, _gloffset_FogCoorddvEXT)
	GL_STUB(glFogCoordfEXT, _gloffset_FogCoordfEXT)
	GL_STUB(glFogCoordfvEXT, _gloffset_FogCoordfvEXT)
	GL_STUB(gl_dispatch_stub_606, _gloffset__dispatch_stub_606)
	GL_STUB(glBlendFuncSeparateEXT, _gloffset_BlendFuncSeparateEXT)
	GL_STUB(glFlushVertexArrayRangeNV, _gloffset_FlushVertexArrayRangeNV)
	GL_STUB(glVertexArrayRangeNV, _gloffset_VertexArrayRangeNV)
	GL_STUB(glCombinerInputNV, _gloffset_CombinerInputNV)
	GL_STUB(glCombinerOutputNV, _gloffset_CombinerOutputNV)
	GL_STUB(glCombinerParameterfNV, _gloffset_CombinerParameterfNV)
	GL_STUB(glCombinerParameterfvNV, _gloffset_CombinerParameterfvNV)
	GL_STUB(glCombinerParameteriNV, _gloffset_CombinerParameteriNV)
	GL_STUB(glCombinerParameterivNV, _gloffset_CombinerParameterivNV)
	GL_STUB(glFinalCombinerInputNV, _gloffset_FinalCombinerInputNV)
	GL_STUB(glGetCombinerInputParameterfvNV, _gloffset_GetCombinerInputParameterfvNV)
	GL_STUB(glGetCombinerInputParameterivNV, _gloffset_GetCombinerInputParameterivNV)
	GL_STUB(glGetCombinerOutputParameterfvNV, _gloffset_GetCombinerOutputParameterfvNV)
	GL_STUB(glGetCombinerOutputParameterivNV, _gloffset_GetCombinerOutputParameterivNV)
	GL_STUB(glGetFinalCombinerInputParameterfvNV, _gloffset_GetFinalCombinerInputParameterfvNV)
	GL_STUB(glGetFinalCombinerInputParameterivNV, _gloffset_GetFinalCombinerInputParameterivNV)
	GL_STUB(glResizeBuffersMESA, _gloffset_ResizeBuffersMESA)
	GL_STUB(glWindowPos2dMESA, _gloffset_WindowPos2dMESA)
	GL_STUB(glWindowPos2dvMESA, _gloffset_WindowPos2dvMESA)
	GL_STUB(glWindowPos2fMESA, _gloffset_WindowPos2fMESA)
	GL_STUB(glWindowPos2fvMESA, _gloffset_WindowPos2fvMESA)
	GL_STUB(glWindowPos2iMESA, _gloffset_WindowPos2iMESA)
	GL_STUB(glWindowPos2ivMESA, _gloffset_WindowPos2ivMESA)
	GL_STUB(glWindowPos2sMESA, _gloffset_WindowPos2sMESA)
	GL_STUB(glWindowPos2svMESA, _gloffset_WindowPos2svMESA)
	GL_STUB(glWindowPos3dMESA, _gloffset_WindowPos3dMESA)
	GL_STUB(glWindowPos3dvMESA, _gloffset_WindowPos3dvMESA)
	GL_STUB(glWindowPos3fMESA, _gloffset_WindowPos3fMESA)
	GL_STUB(glWindowPos3fvMESA, _gloffset_WindowPos3fvMESA)
	GL_STUB(glWindowPos3iMESA, _gloffset_WindowPos3iMESA)
	GL_STUB(glWindowPos3ivMESA, _gloffset_WindowPos3ivMESA)
	GL_STUB(glWindowPos3sMESA, _gloffset_WindowPos3sMESA)
	GL_STUB(glWindowPos3svMESA, _gloffset_WindowPos3svMESA)
	GL_STUB(glWindowPos4dMESA, _gloffset_WindowPos4dMESA)
	GL_STUB(glWindowPos4dvMESA, _gloffset_WindowPos4dvMESA)
	GL_STUB(glWindowPos4fMESA, _gloffset_WindowPos4fMESA)
	GL_STUB(glWindowPos4fvMESA, _gloffset_WindowPos4fvMESA)
	GL_STUB(glWindowPos4iMESA, _gloffset_WindowPos4iMESA)
	GL_STUB(glWindowPos4ivMESA, _gloffset_WindowPos4ivMESA)
	GL_STUB(glWindowPos4sMESA, _gloffset_WindowPos4sMESA)
	GL_STUB(glWindowPos4svMESA, _gloffset_WindowPos4svMESA)
	GL_STUB(gl_dispatch_stub_648, _gloffset__dispatch_stub_648)
	GL_STUB(gl_dispatch_stub_649, _gloffset__dispatch_stub_649)
	GL_STUB(gl_dispatch_stub_650, _gloffset__dispatch_stub_650)
	GL_STUB(gl_dispatch_stub_651, _gloffset__dispatch_stub_651)
	GL_STUB(gl_dispatch_stub_652, _gloffset__dispatch_stub_652)
	GL_STUB(gl_dispatch_stub_653, _gloffset__dispatch_stub_653)
	GL_STUB(gl_dispatch_stub_654, _gloffset__dispatch_stub_654)
	GL_STUB(gl_dispatch_stub_655, _gloffset__dispatch_stub_655)
	GL_STUB(gl_dispatch_stub_656, _gloffset__dispatch_stub_656)
	GL_STUB(glAreProgramsResidentNV, _gloffset_AreProgramsResidentNV)
	GL_STUB(glBindProgramNV, _gloffset_BindProgramNV)
	GL_STUB(glDeleteProgramsNV, _gloffset_DeleteProgramsNV)
	GL_STUB(glExecuteProgramNV, _gloffset_ExecuteProgramNV)
	GL_STUB(glGenProgramsNV, _gloffset_GenProgramsNV)
	GL_STUB(glGetProgramParameterdvNV, _gloffset_GetProgramParameterdvNV)
	GL_STUB(glGetProgramParameterfvNV, _gloffset_GetProgramParameterfvNV)
	GL_STUB(glGetProgramStringNV, _gloffset_GetProgramStringNV)
	GL_STUB(glGetProgramivNV, _gloffset_GetProgramivNV)
	GL_STUB(glGetTrackMatrixivNV, _gloffset_GetTrackMatrixivNV)
	GL_STUB(glGetVertexAttribPointervNV, _gloffset_GetVertexAttribPointervNV)
	GL_STUB(glGetVertexAttribdvNV, _gloffset_GetVertexAttribdvNV)
	GL_STUB(glGetVertexAttribfvNV, _gloffset_GetVertexAttribfvNV)
	GL_STUB(glGetVertexAttribivNV, _gloffset_GetVertexAttribivNV)
	GL_STUB(glIsProgramNV, _gloffset_IsProgramNV)
	GL_STUB(glLoadProgramNV, _gloffset_LoadProgramNV)
	GL_STUB(glProgramParameter4dNV, _gloffset_ProgramParameter4dNV)
	GL_STUB(glProgramParameter4dvNV, _gloffset_ProgramParameter4dvNV)
	GL_STUB(glProgramParameter4fNV, _gloffset_ProgramParameter4fNV)
	GL_STUB(glProgramParameter4fvNV, _gloffset_ProgramParameter4fvNV)
	GL_STUB(glProgramParameters4dvNV, _gloffset_ProgramParameters4dvNV)
	GL_STUB(glProgramParameters4fvNV, _gloffset_ProgramParameters4fvNV)
	GL_STUB(glRequestResidentProgramsNV, _gloffset_RequestResidentProgramsNV)
	GL_STUB(glTrackMatrixNV, _gloffset_TrackMatrixNV)
	GL_STUB(glVertexAttrib1dNV, _gloffset_VertexAttrib1dNV)
	GL_STUB(glVertexAttrib1dvNV, _gloffset_VertexAttrib1dvNV)
	GL_STUB(glVertexAttrib1fNV, _gloffset_VertexAttrib1fNV)
	GL_STUB(glVertexAttrib1fvNV, _gloffset_VertexAttrib1fvNV)
	GL_STUB(glVertexAttrib1sNV, _gloffset_VertexAttrib1sNV)
	GL_STUB(glVertexAttrib1svNV, _gloffset_VertexAttrib1svNV)
	GL_STUB(glVertexAttrib2dNV, _gloffset_VertexAttrib2dNV)
	GL_STUB(glVertexAttrib2dvNV, _gloffset_VertexAttrib2dvNV)
	GL_STUB(glVertexAttrib2fNV, _gloffset_VertexAttrib2fNV)
	GL_STUB(glVertexAttrib2fvNV, _gloffset_VertexAttrib2fvNV)
	GL_STUB(glVertexAttrib2sNV, _gloffset_VertexAttrib2sNV)
	GL_STUB(glVertexAttrib2svNV, _gloffset_VertexAttrib2svNV)
	GL_STUB(glVertexAttrib3dNV, _gloffset_VertexAttrib3dNV)
	GL_STUB(glVertexAttrib3dvNV, _gloffset_VertexAttrib3dvNV)
	GL_STUB(glVertexAttrib3fNV, _gloffset_VertexAttrib3fNV)
	GL_STUB(glVertexAttrib3fvNV, _gloffset_VertexAttrib3fvNV)
	GL_STUB(glVertexAttrib3sNV, _gloffset_VertexAttrib3sNV)
	GL_STUB(glVertexAttrib3svNV, _gloffset_VertexAttrib3svNV)
	GL_STUB(glVertexAttrib4dNV, _gloffset_VertexAttrib4dNV)
	GL_STUB(glVertexAttrib4dvNV, _gloffset_VertexAttrib4dvNV)
	GL_STUB(glVertexAttrib4fNV, _gloffset_VertexAttrib4fNV)
	GL_STUB(glVertexAttrib4fvNV, _gloffset_VertexAttrib4fvNV)
	GL_STUB(glVertexAttrib4sNV, _gloffset_VertexAttrib4sNV)
	GL_STUB(glVertexAttrib4svNV, _gloffset_VertexAttrib4svNV)
	GL_STUB(glVertexAttrib4ubNV, _gloffset_VertexAttrib4ubNV)
	GL_STUB(glVertexAttrib4ubvNV, _gloffset_VertexAttrib4ubvNV)
	GL_STUB(glVertexAttribPointerNV, _gloffset_VertexAttribPointerNV)
	GL_STUB(glVertexAttribs1dvNV, _gloffset_VertexAttribs1dvNV)
	GL_STUB(glVertexAttribs1fvNV, _gloffset_VertexAttribs1fvNV)
	GL_STUB(glVertexAttribs1svNV, _gloffset_VertexAttribs1svNV)
	GL_STUB(glVertexAttribs2dvNV, _gloffset_VertexAttribs2dvNV)
	GL_STUB(glVertexAttribs2fvNV, _gloffset_VertexAttribs2fvNV)
	GL_STUB(glVertexAttribs2svNV, _gloffset_VertexAttribs2svNV)
	GL_STUB(glVertexAttribs3dvNV, _gloffset_VertexAttribs3dvNV)
	GL_STUB(glVertexAttribs3fvNV, _gloffset_VertexAttribs3fvNV)
	GL_STUB(glVertexAttribs3svNV, _gloffset_VertexAttribs3svNV)
	GL_STUB(glVertexAttribs4dvNV, _gloffset_VertexAttribs4dvNV)
	GL_STUB(glVertexAttribs4fvNV, _gloffset_VertexAttribs4fvNV)
	GL_STUB(glVertexAttribs4svNV, _gloffset_VertexAttribs4svNV)
	GL_STUB(glVertexAttribs4ubvNV, _gloffset_VertexAttribs4ubvNV)
	GL_STUB(glAlphaFragmentOp1ATI, _gloffset_AlphaFragmentOp1ATI)
	GL_STUB(glAlphaFragmentOp2ATI, _gloffset_AlphaFragmentOp2ATI)
	GL_STUB(glAlphaFragmentOp3ATI, _gloffset_AlphaFragmentOp3ATI)
	GL_STUB(glBeginFragmentShaderATI, _gloffset_BeginFragmentShaderATI)
	GL_STUB(glBindFragmentShaderATI, _gloffset_BindFragmentShaderATI)
	GL_STUB(glColorFragmentOp1ATI, _gloffset_ColorFragmentOp1ATI)
	GL_STUB(glColorFragmentOp2ATI, _gloffset_ColorFragmentOp2ATI)
	GL_STUB(glColorFragmentOp3ATI, _gloffset_ColorFragmentOp3ATI)
	GL_STUB(glDeleteFragmentShaderATI, _gloffset_DeleteFragmentShaderATI)
	GL_STUB(glEndFragmentShaderATI, _gloffset_EndFragmentShaderATI)
	GL_STUB(glGenFragmentShadersATI, _gloffset_GenFragmentShadersATI)
	GL_STUB(glPassTexCoordATI, _gloffset_PassTexCoordATI)
	GL_STUB(glSampleMapATI, _gloffset_SampleMapATI)
	GL_STUB(glSetFragmentShaderConstantATI, _gloffset_SetFragmentShaderConstantATI)
	GL_STUB(glPointParameteriNV, _gloffset_PointParameteriNV)
	GL_STUB(glPointParameterivNV, _gloffset_PointParameterivNV)
	GL_STUB(gl_dispatch_stub_737, _gloffset__dispatch_stub_737)
	GL_STUB(gl_dispatch_stub_738, _gloffset__dispatch_stub_738)
	GL_STUB(gl_dispatch_stub_739, _gloffset__dispatch_stub_739)
	GL_STUB(gl_dispatch_stub_740, _gloffset__dispatch_stub_740)
	GL_STUB(gl_dispatch_stub_741, _gloffset__dispatch_stub_741)
	GL_STUB(glGetProgramNamedParameterdvNV, _gloffset_GetProgramNamedParameterdvNV)
	GL_STUB(glGetProgramNamedParameterfvNV, _gloffset_GetProgramNamedParameterfvNV)
	GL_STUB(glProgramNamedParameter4dNV, _gloffset_ProgramNamedParameter4dNV)
	GL_STUB(glProgramNamedParameter4dvNV, _gloffset_ProgramNamedParameter4dvNV)
	GL_STUB(glProgramNamedParameter4fNV, _gloffset_ProgramNamedParameter4fNV)
	GL_STUB(glProgramNamedParameter4fvNV, _gloffset_ProgramNamedParameter4fvNV)
	GL_STUB(gl_dispatch_stub_748, _gloffset__dispatch_stub_748)
	GL_STUB(gl_dispatch_stub_749, _gloffset__dispatch_stub_749)
	GL_STUB(glBindFramebufferEXT, _gloffset_BindFramebufferEXT)
	GL_STUB(glBindRenderbufferEXT, _gloffset_BindRenderbufferEXT)
	GL_STUB(glCheckFramebufferStatusEXT, _gloffset_CheckFramebufferStatusEXT)
	GL_STUB(glDeleteFramebuffersEXT, _gloffset_DeleteFramebuffersEXT)
	GL_STUB(glDeleteRenderbuffersEXT, _gloffset_DeleteRenderbuffersEXT)
	GL_STUB(glFramebufferRenderbufferEXT, _gloffset_FramebufferRenderbufferEXT)
	GL_STUB(glFramebufferTexture1DEXT, _gloffset_FramebufferTexture1DEXT)
	GL_STUB(glFramebufferTexture2DEXT, _gloffset_FramebufferTexture2DEXT)
	GL_STUB(glFramebufferTexture3DEXT, _gloffset_FramebufferTexture3DEXT)
	GL_STUB(glGenFramebuffersEXT, _gloffset_GenFramebuffersEXT)
	GL_STUB(glGenRenderbuffersEXT, _gloffset_GenRenderbuffersEXT)
	GL_STUB(glGenerateMipmapEXT, _gloffset_GenerateMipmapEXT)
	GL_STUB(glGetFramebufferAttachmentParameterivEXT, _gloffset_GetFramebufferAttachmentParameterivEXT)
	GL_STUB(glGetRenderbufferParameterivEXT, _gloffset_GetRenderbufferParameterivEXT)
	GL_STUB(glIsFramebufferEXT, _gloffset_IsFramebufferEXT)
	GL_STUB(glIsRenderbufferEXT, _gloffset_IsRenderbufferEXT)
	GL_STUB(glRenderbufferStorageEXT, _gloffset_RenderbufferStorageEXT)
	GL_STUB(gl_dispatch_stub_767, _gloffset__dispatch_stub_767)
	GL_STUB(gl_dispatch_stub_768, _gloffset__dispatch_stub_768)
	GL_STUB(gl_dispatch_stub_769, _gloffset__dispatch_stub_769)
	GL_STUB(gl_dispatch_stub_770, _gloffset__dispatch_stub_770)
	GL_STUB(gl_dispatch_stub_771, _gloffset__dispatch_stub_771)
	GL_STUB(gl_dispatch_stub_772, _gloffset__dispatch_stub_772)

		.globl _mesa_sparc_glapi_end ; .type _mesa_sparc_glapi_end,#function
_mesa_sparc_glapi_end:

	.globl glArrayElementEXT ; .type glArrayElementEXT,#function ; glArrayElementEXT = glArrayElement
	.globl glBindTextureEXT ; .type glBindTextureEXT,#function ; glBindTextureEXT = glBindTexture
	.globl glDrawArraysEXT ; .type glDrawArraysEXT,#function ; glDrawArraysEXT = glDrawArrays
#ifndef GLX_INDIRECT_RENDERING
	.globl glAreTexturesResidentEXT ; .type glAreTexturesResidentEXT,#function ; glAreTexturesResidentEXT = glAreTexturesResident
#endif
	.globl glCopyTexImage1DEXT ; .type glCopyTexImage1DEXT,#function ; glCopyTexImage1DEXT = glCopyTexImage1D
	.globl glCopyTexImage2DEXT ; .type glCopyTexImage2DEXT,#function ; glCopyTexImage2DEXT = glCopyTexImage2D
	.globl glCopyTexSubImage1DEXT ; .type glCopyTexSubImage1DEXT,#function ; glCopyTexSubImage1DEXT = glCopyTexSubImage1D
	.globl glCopyTexSubImage2DEXT ; .type glCopyTexSubImage2DEXT,#function ; glCopyTexSubImage2DEXT = glCopyTexSubImage2D
#ifndef GLX_INDIRECT_RENDERING
	.globl glDeleteTexturesEXT ; .type glDeleteTexturesEXT,#function ; glDeleteTexturesEXT = glDeleteTextures
#endif
#ifndef GLX_INDIRECT_RENDERING
	.globl glGenTexturesEXT ; .type glGenTexturesEXT,#function ; glGenTexturesEXT = glGenTextures
#endif
	.globl glGetPointervEXT ; .type glGetPointervEXT,#function ; glGetPointervEXT = glGetPointerv
#ifndef GLX_INDIRECT_RENDERING
	.globl glIsTextureEXT ; .type glIsTextureEXT,#function ; glIsTextureEXT = glIsTexture
#endif
	.globl glPrioritizeTexturesEXT ; .type glPrioritizeTexturesEXT,#function ; glPrioritizeTexturesEXT = glPrioritizeTextures
	.globl glTexSubImage1DEXT ; .type glTexSubImage1DEXT,#function ; glTexSubImage1DEXT = glTexSubImage1D
	.globl glTexSubImage2DEXT ; .type glTexSubImage2DEXT,#function ; glTexSubImage2DEXT = glTexSubImage2D
	.globl glBlendColorEXT ; .type glBlendColorEXT,#function ; glBlendColorEXT = glBlendColor
	.globl glBlendEquationEXT ; .type glBlendEquationEXT,#function ; glBlendEquationEXT = glBlendEquation
	.globl glDrawRangeElementsEXT ; .type glDrawRangeElementsEXT,#function ; glDrawRangeElementsEXT = glDrawRangeElements
	.globl glColorTableEXT ; .type glColorTableEXT,#function ; glColorTableEXT = glColorTable
#ifndef GLX_INDIRECT_RENDERING
	.globl glGetColorTableEXT ; .type glGetColorTableEXT,#function ; glGetColorTableEXT = glGetColorTable
#endif
#ifndef GLX_INDIRECT_RENDERING
	.globl glGetColorTableParameterfvEXT ; .type glGetColorTableParameterfvEXT,#function ; glGetColorTableParameterfvEXT = glGetColorTableParameterfv
#endif
#ifndef GLX_INDIRECT_RENDERING
	.globl glGetColorTableParameterivEXT ; .type glGetColorTableParameterivEXT,#function ; glGetColorTableParameterivEXT = glGetColorTableParameteriv
#endif
	.globl glTexImage3DEXT ; .type glTexImage3DEXT,#function ; glTexImage3DEXT = glTexImage3D
	.globl glTexSubImage3DEXT ; .type glTexSubImage3DEXT,#function ; glTexSubImage3DEXT = glTexSubImage3D
	.globl glCopyTexSubImage3DEXT ; .type glCopyTexSubImage3DEXT,#function ; glCopyTexSubImage3DEXT = glCopyTexSubImage3D
	.globl glActiveTexture ; .type glActiveTexture,#function ; glActiveTexture = glActiveTextureARB
	.globl glClientActiveTexture ; .type glClientActiveTexture,#function ; glClientActiveTexture = glClientActiveTextureARB
	.globl glMultiTexCoord1d ; .type glMultiTexCoord1d,#function ; glMultiTexCoord1d = glMultiTexCoord1dARB
	.globl glMultiTexCoord1dv ; .type glMultiTexCoord1dv,#function ; glMultiTexCoord1dv = glMultiTexCoord1dvARB
	.globl glMultiTexCoord1f ; .type glMultiTexCoord1f,#function ; glMultiTexCoord1f = glMultiTexCoord1fARB
	.globl glMultiTexCoord1fv ; .type glMultiTexCoord1fv,#function ; glMultiTexCoord1fv = glMultiTexCoord1fvARB
	.globl glMultiTexCoord1i ; .type glMultiTexCoord1i,#function ; glMultiTexCoord1i = glMultiTexCoord1iARB
	.globl glMultiTexCoord1iv ; .type glMultiTexCoord1iv,#function ; glMultiTexCoord1iv = glMultiTexCoord1ivARB
	.globl glMultiTexCoord1s ; .type glMultiTexCoord1s,#function ; glMultiTexCoord1s = glMultiTexCoord1sARB
	.globl glMultiTexCoord1sv ; .type glMultiTexCoord1sv,#function ; glMultiTexCoord1sv = glMultiTexCoord1svARB
	.globl glMultiTexCoord2d ; .type glMultiTexCoord2d,#function ; glMultiTexCoord2d = glMultiTexCoord2dARB
	.globl glMultiTexCoord2dv ; .type glMultiTexCoord2dv,#function ; glMultiTexCoord2dv = glMultiTexCoord2dvARB
	.globl glMultiTexCoord2f ; .type glMultiTexCoord2f,#function ; glMultiTexCoord2f = glMultiTexCoord2fARB
	.globl glMultiTexCoord2fv ; .type glMultiTexCoord2fv,#function ; glMultiTexCoord2fv = glMultiTexCoord2fvARB
	.globl glMultiTexCoord2i ; .type glMultiTexCoord2i,#function ; glMultiTexCoord2i = glMultiTexCoord2iARB
	.globl glMultiTexCoord2iv ; .type glMultiTexCoord2iv,#function ; glMultiTexCoord2iv = glMultiTexCoord2ivARB
	.globl glMultiTexCoord2s ; .type glMultiTexCoord2s,#function ; glMultiTexCoord2s = glMultiTexCoord2sARB
	.globl glMultiTexCoord2sv ; .type glMultiTexCoord2sv,#function ; glMultiTexCoord2sv = glMultiTexCoord2svARB
	.globl glMultiTexCoord3d ; .type glMultiTexCoord3d,#function ; glMultiTexCoord3d = glMultiTexCoord3dARB
	.globl glMultiTexCoord3dv ; .type glMultiTexCoord3dv,#function ; glMultiTexCoord3dv = glMultiTexCoord3dvARB
	.globl glMultiTexCoord3f ; .type glMultiTexCoord3f,#function ; glMultiTexCoord3f = glMultiTexCoord3fARB
	.globl glMultiTexCoord3fv ; .type glMultiTexCoord3fv,#function ; glMultiTexCoord3fv = glMultiTexCoord3fvARB
	.globl glMultiTexCoord3i ; .type glMultiTexCoord3i,#function ; glMultiTexCoord3i = glMultiTexCoord3iARB
	.globl glMultiTexCoord3iv ; .type glMultiTexCoord3iv,#function ; glMultiTexCoord3iv = glMultiTexCoord3ivARB
	.globl glMultiTexCoord3s ; .type glMultiTexCoord3s,#function ; glMultiTexCoord3s = glMultiTexCoord3sARB
	.globl glMultiTexCoord3sv ; .type glMultiTexCoord3sv,#function ; glMultiTexCoord3sv = glMultiTexCoord3svARB
	.globl glMultiTexCoord4d ; .type glMultiTexCoord4d,#function ; glMultiTexCoord4d = glMultiTexCoord4dARB
	.globl glMultiTexCoord4dv ; .type glMultiTexCoord4dv,#function ; glMultiTexCoord4dv = glMultiTexCoord4dvARB
	.globl glMultiTexCoord4f ; .type glMultiTexCoord4f,#function ; glMultiTexCoord4f = glMultiTexCoord4fARB
	.globl glMultiTexCoord4fv ; .type glMultiTexCoord4fv,#function ; glMultiTexCoord4fv = glMultiTexCoord4fvARB
	.globl glMultiTexCoord4i ; .type glMultiTexCoord4i,#function ; glMultiTexCoord4i = glMultiTexCoord4iARB
	.globl glMultiTexCoord4iv ; .type glMultiTexCoord4iv,#function ; glMultiTexCoord4iv = glMultiTexCoord4ivARB
	.globl glMultiTexCoord4s ; .type glMultiTexCoord4s,#function ; glMultiTexCoord4s = glMultiTexCoord4sARB
	.globl glMultiTexCoord4sv ; .type glMultiTexCoord4sv,#function ; glMultiTexCoord4sv = glMultiTexCoord4svARB
	.globl glLoadTransposeMatrixd ; .type glLoadTransposeMatrixd,#function ; glLoadTransposeMatrixd = glLoadTransposeMatrixdARB
	.globl glLoadTransposeMatrixf ; .type glLoadTransposeMatrixf,#function ; glLoadTransposeMatrixf = glLoadTransposeMatrixfARB
	.globl glMultTransposeMatrixd ; .type glMultTransposeMatrixd,#function ; glMultTransposeMatrixd = glMultTransposeMatrixdARB
	.globl glMultTransposeMatrixf ; .type glMultTransposeMatrixf,#function ; glMultTransposeMatrixf = glMultTransposeMatrixfARB
	.globl glSampleCoverage ; .type glSampleCoverage,#function ; glSampleCoverage = glSampleCoverageARB
	.globl glCompressedTexImage1D ; .type glCompressedTexImage1D,#function ; glCompressedTexImage1D = glCompressedTexImage1DARB
	.globl glCompressedTexImage2D ; .type glCompressedTexImage2D,#function ; glCompressedTexImage2D = glCompressedTexImage2DARB
	.globl glCompressedTexImage3D ; .type glCompressedTexImage3D,#function ; glCompressedTexImage3D = glCompressedTexImage3DARB
	.globl glCompressedTexSubImage1D ; .type glCompressedTexSubImage1D,#function ; glCompressedTexSubImage1D = glCompressedTexSubImage1DARB
	.globl glCompressedTexSubImage2D ; .type glCompressedTexSubImage2D,#function ; glCompressedTexSubImage2D = glCompressedTexSubImage2DARB
	.globl glCompressedTexSubImage3D ; .type glCompressedTexSubImage3D,#function ; glCompressedTexSubImage3D = glCompressedTexSubImage3DARB
	.globl glGetCompressedTexImage ; .type glGetCompressedTexImage,#function ; glGetCompressedTexImage = glGetCompressedTexImageARB
	.globl glDisableVertexAttribArray ; .type glDisableVertexAttribArray,#function ; glDisableVertexAttribArray = glDisableVertexAttribArrayARB
	.globl glEnableVertexAttribArray ; .type glEnableVertexAttribArray,#function ; glEnableVertexAttribArray = glEnableVertexAttribArrayARB
	.globl glGetVertexAttribdv ; .type glGetVertexAttribdv,#function ; glGetVertexAttribdv = glGetVertexAttribdvARB
	.globl glGetVertexAttribfv ; .type glGetVertexAttribfv,#function ; glGetVertexAttribfv = glGetVertexAttribfvARB
	.globl glGetVertexAttribiv ; .type glGetVertexAttribiv,#function ; glGetVertexAttribiv = glGetVertexAttribivARB
	.globl glVertexAttrib1d ; .type glVertexAttrib1d,#function ; glVertexAttrib1d = glVertexAttrib1dARB
	.globl glVertexAttrib1dv ; .type glVertexAttrib1dv,#function ; glVertexAttrib1dv = glVertexAttrib1dvARB
	.globl glVertexAttrib1f ; .type glVertexAttrib1f,#function ; glVertexAttrib1f = glVertexAttrib1fARB
	.globl glVertexAttrib1fv ; .type glVertexAttrib1fv,#function ; glVertexAttrib1fv = glVertexAttrib1fvARB
	.globl glVertexAttrib1s ; .type glVertexAttrib1s,#function ; glVertexAttrib1s = glVertexAttrib1sARB
	.globl glVertexAttrib1sv ; .type glVertexAttrib1sv,#function ; glVertexAttrib1sv = glVertexAttrib1svARB
	.globl glVertexAttrib2d ; .type glVertexAttrib2d,#function ; glVertexAttrib2d = glVertexAttrib2dARB
	.globl glVertexAttrib2dv ; .type glVertexAttrib2dv,#function ; glVertexAttrib2dv = glVertexAttrib2dvARB
	.globl glVertexAttrib2f ; .type glVertexAttrib2f,#function ; glVertexAttrib2f = glVertexAttrib2fARB
	.globl glVertexAttrib2fv ; .type glVertexAttrib2fv,#function ; glVertexAttrib2fv = glVertexAttrib2fvARB
	.globl glVertexAttrib2s ; .type glVertexAttrib2s,#function ; glVertexAttrib2s = glVertexAttrib2sARB
	.globl glVertexAttrib2sv ; .type glVertexAttrib2sv,#function ; glVertexAttrib2sv = glVertexAttrib2svARB
	.globl glVertexAttrib3d ; .type glVertexAttrib3d,#function ; glVertexAttrib3d = glVertexAttrib3dARB
	.globl glVertexAttrib3dv ; .type glVertexAttrib3dv,#function ; glVertexAttrib3dv = glVertexAttrib3dvARB
	.globl glVertexAttrib3f ; .type glVertexAttrib3f,#function ; glVertexAttrib3f = glVertexAttrib3fARB
	.globl glVertexAttrib3fv ; .type glVertexAttrib3fv,#function ; glVertexAttrib3fv = glVertexAttrib3fvARB
	.globl glVertexAttrib3s ; .type glVertexAttrib3s,#function ; glVertexAttrib3s = glVertexAttrib3sARB
	.globl glVertexAttrib3sv ; .type glVertexAttrib3sv,#function ; glVertexAttrib3sv = glVertexAttrib3svARB
	.globl glVertexAttrib4Nbv ; .type glVertexAttrib4Nbv,#function ; glVertexAttrib4Nbv = glVertexAttrib4NbvARB
	.globl glVertexAttrib4Niv ; .type glVertexAttrib4Niv,#function ; glVertexAttrib4Niv = glVertexAttrib4NivARB
	.globl glVertexAttrib4Nsv ; .type glVertexAttrib4Nsv,#function ; glVertexAttrib4Nsv = glVertexAttrib4NsvARB
	.globl glVertexAttrib4Nub ; .type glVertexAttrib4Nub,#function ; glVertexAttrib4Nub = glVertexAttrib4NubARB
	.globl glVertexAttrib4Nubv ; .type glVertexAttrib4Nubv,#function ; glVertexAttrib4Nubv = glVertexAttrib4NubvARB
	.globl glVertexAttrib4Nuiv ; .type glVertexAttrib4Nuiv,#function ; glVertexAttrib4Nuiv = glVertexAttrib4NuivARB
	.globl glVertexAttrib4Nusv ; .type glVertexAttrib4Nusv,#function ; glVertexAttrib4Nusv = glVertexAttrib4NusvARB
	.globl glVertexAttrib4bv ; .type glVertexAttrib4bv,#function ; glVertexAttrib4bv = glVertexAttrib4bvARB
	.globl glVertexAttrib4d ; .type glVertexAttrib4d,#function ; glVertexAttrib4d = glVertexAttrib4dARB
	.globl glVertexAttrib4dv ; .type glVertexAttrib4dv,#function ; glVertexAttrib4dv = glVertexAttrib4dvARB
	.globl glVertexAttrib4f ; .type glVertexAttrib4f,#function ; glVertexAttrib4f = glVertexAttrib4fARB
	.globl glVertexAttrib4fv ; .type glVertexAttrib4fv,#function ; glVertexAttrib4fv = glVertexAttrib4fvARB
	.globl glVertexAttrib4iv ; .type glVertexAttrib4iv,#function ; glVertexAttrib4iv = glVertexAttrib4ivARB
	.globl glVertexAttrib4s ; .type glVertexAttrib4s,#function ; glVertexAttrib4s = glVertexAttrib4sARB
	.globl glVertexAttrib4sv ; .type glVertexAttrib4sv,#function ; glVertexAttrib4sv = glVertexAttrib4svARB
	.globl glVertexAttrib4ubv ; .type glVertexAttrib4ubv,#function ; glVertexAttrib4ubv = glVertexAttrib4ubvARB
	.globl glVertexAttrib4uiv ; .type glVertexAttrib4uiv,#function ; glVertexAttrib4uiv = glVertexAttrib4uivARB
	.globl glVertexAttrib4usv ; .type glVertexAttrib4usv,#function ; glVertexAttrib4usv = glVertexAttrib4usvARB
	.globl glVertexAttribPointer ; .type glVertexAttribPointer,#function ; glVertexAttribPointer = glVertexAttribPointerARB
	.globl glBindBuffer ; .type glBindBuffer,#function ; glBindBuffer = glBindBufferARB
	.globl glBufferData ; .type glBufferData,#function ; glBufferData = glBufferDataARB
	.globl glBufferSubData ; .type glBufferSubData,#function ; glBufferSubData = glBufferSubDataARB
	.globl glDeleteBuffers ; .type glDeleteBuffers,#function ; glDeleteBuffers = glDeleteBuffersARB
	.globl glGenBuffers ; .type glGenBuffers,#function ; glGenBuffers = glGenBuffersARB
	.globl glGetBufferParameteriv ; .type glGetBufferParameteriv,#function ; glGetBufferParameteriv = glGetBufferParameterivARB
	.globl glGetBufferPointerv ; .type glGetBufferPointerv,#function ; glGetBufferPointerv = glGetBufferPointervARB
	.globl glGetBufferSubData ; .type glGetBufferSubData,#function ; glGetBufferSubData = glGetBufferSubDataARB
	.globl glIsBuffer ; .type glIsBuffer,#function ; glIsBuffer = glIsBufferARB
	.globl glMapBuffer ; .type glMapBuffer,#function ; glMapBuffer = glMapBufferARB
	.globl glUnmapBuffer ; .type glUnmapBuffer,#function ; glUnmapBuffer = glUnmapBufferARB
	.globl glBeginQuery ; .type glBeginQuery,#function ; glBeginQuery = glBeginQueryARB
	.globl glDeleteQueries ; .type glDeleteQueries,#function ; glDeleteQueries = glDeleteQueriesARB
	.globl glEndQuery ; .type glEndQuery,#function ; glEndQuery = glEndQueryARB
	.globl glGenQueries ; .type glGenQueries,#function ; glGenQueries = glGenQueriesARB
	.globl glGetQueryObjectiv ; .type glGetQueryObjectiv,#function ; glGetQueryObjectiv = glGetQueryObjectivARB
	.globl glGetQueryObjectuiv ; .type glGetQueryObjectuiv,#function ; glGetQueryObjectuiv = glGetQueryObjectuivARB
	.globl glGetQueryiv ; .type glGetQueryiv,#function ; glGetQueryiv = glGetQueryivARB
	.globl glIsQuery ; .type glIsQuery,#function ; glIsQuery = glIsQueryARB
	.globl glCompileShader ; .type glCompileShader,#function ; glCompileShader = glCompileShaderARB
	.globl glGetActiveUniform ; .type glGetActiveUniform,#function ; glGetActiveUniform = glGetActiveUniformARB
	.globl glGetShaderSource ; .type glGetShaderSource,#function ; glGetShaderSource = glGetShaderSourceARB
	.globl glGetUniformLocation ; .type glGetUniformLocation,#function ; glGetUniformLocation = glGetUniformLocationARB
	.globl glGetUniformfv ; .type glGetUniformfv,#function ; glGetUniformfv = glGetUniformfvARB
	.globl glGetUniformiv ; .type glGetUniformiv,#function ; glGetUniformiv = glGetUniformivARB
	.globl glLinkProgram ; .type glLinkProgram,#function ; glLinkProgram = glLinkProgramARB
	.globl glShaderSource ; .type glShaderSource,#function ; glShaderSource = glShaderSourceARB
	.globl glUniform1f ; .type glUniform1f,#function ; glUniform1f = glUniform1fARB
	.globl glUniform1fv ; .type glUniform1fv,#function ; glUniform1fv = glUniform1fvARB
	.globl glUniform1i ; .type glUniform1i,#function ; glUniform1i = glUniform1iARB
	.globl glUniform1iv ; .type glUniform1iv,#function ; glUniform1iv = glUniform1ivARB
	.globl glUniform2f ; .type glUniform2f,#function ; glUniform2f = glUniform2fARB
	.globl glUniform2fv ; .type glUniform2fv,#function ; glUniform2fv = glUniform2fvARB
	.globl glUniform2i ; .type glUniform2i,#function ; glUniform2i = glUniform2iARB
	.globl glUniform2iv ; .type glUniform2iv,#function ; glUniform2iv = glUniform2ivARB
	.globl glUniform3f ; .type glUniform3f,#function ; glUniform3f = glUniform3fARB
	.globl glUniform3fv ; .type glUniform3fv,#function ; glUniform3fv = glUniform3fvARB
	.globl glUniform3i ; .type glUniform3i,#function ; glUniform3i = glUniform3iARB
	.globl glUniform3iv ; .type glUniform3iv,#function ; glUniform3iv = glUniform3ivARB
	.globl glUniform4f ; .type glUniform4f,#function ; glUniform4f = glUniform4fARB
	.globl glUniform4fv ; .type glUniform4fv,#function ; glUniform4fv = glUniform4fvARB
	.globl glUniform4i ; .type glUniform4i,#function ; glUniform4i = glUniform4iARB
	.globl glUniform4iv ; .type glUniform4iv,#function ; glUniform4iv = glUniform4ivARB
	.globl glUniformMatrix2fv ; .type glUniformMatrix2fv,#function ; glUniformMatrix2fv = glUniformMatrix2fvARB
	.globl glUniformMatrix3fv ; .type glUniformMatrix3fv,#function ; glUniformMatrix3fv = glUniformMatrix3fvARB
	.globl glUniformMatrix4fv ; .type glUniformMatrix4fv,#function ; glUniformMatrix4fv = glUniformMatrix4fvARB
	.globl glUseProgram ; .type glUseProgram,#function ; glUseProgram = glUseProgramObjectARB
	.globl glValidateProgram ; .type glValidateProgram,#function ; glValidateProgram = glValidateProgramARB
	.globl glBindAttribLocation ; .type glBindAttribLocation,#function ; glBindAttribLocation = glBindAttribLocationARB
	.globl glGetActiveAttrib ; .type glGetActiveAttrib,#function ; glGetActiveAttrib = glGetActiveAttribARB
	.globl glGetAttribLocation ; .type glGetAttribLocation,#function ; glGetAttribLocation = glGetAttribLocationARB
	.globl glDrawBuffers ; .type glDrawBuffers,#function ; glDrawBuffers = glDrawBuffersARB
	.globl glDrawBuffersATI ; .type glDrawBuffersATI,#function ; glDrawBuffersATI = glDrawBuffersARB
	.globl glPointParameterf ; .type glPointParameterf,#function ; glPointParameterf = glPointParameterfEXT
	.globl glPointParameterfARB ; .type glPointParameterfARB,#function ; glPointParameterfARB = glPointParameterfEXT
	.globl glPointParameterfv ; .type glPointParameterfv,#function ; glPointParameterfv = glPointParameterfvEXT
	.globl glPointParameterfvARB ; .type glPointParameterfvARB,#function ; glPointParameterfvARB = glPointParameterfvEXT
	.globl glSecondaryColor3b ; .type glSecondaryColor3b,#function ; glSecondaryColor3b = glSecondaryColor3bEXT
	.globl glSecondaryColor3bv ; .type glSecondaryColor3bv,#function ; glSecondaryColor3bv = glSecondaryColor3bvEXT
	.globl glSecondaryColor3d ; .type glSecondaryColor3d,#function ; glSecondaryColor3d = glSecondaryColor3dEXT
	.globl glSecondaryColor3dv ; .type glSecondaryColor3dv,#function ; glSecondaryColor3dv = glSecondaryColor3dvEXT
	.globl glSecondaryColor3f ; .type glSecondaryColor3f,#function ; glSecondaryColor3f = glSecondaryColor3fEXT
	.globl glSecondaryColor3fv ; .type glSecondaryColor3fv,#function ; glSecondaryColor3fv = glSecondaryColor3fvEXT
	.globl glSecondaryColor3i ; .type glSecondaryColor3i,#function ; glSecondaryColor3i = glSecondaryColor3iEXT
	.globl glSecondaryColor3iv ; .type glSecondaryColor3iv,#function ; glSecondaryColor3iv = glSecondaryColor3ivEXT
	.globl glSecondaryColor3s ; .type glSecondaryColor3s,#function ; glSecondaryColor3s = glSecondaryColor3sEXT
	.globl glSecondaryColor3sv ; .type glSecondaryColor3sv,#function ; glSecondaryColor3sv = glSecondaryColor3svEXT
	.globl glSecondaryColor3ub ; .type glSecondaryColor3ub,#function ; glSecondaryColor3ub = glSecondaryColor3ubEXT
	.globl glSecondaryColor3ubv ; .type glSecondaryColor3ubv,#function ; glSecondaryColor3ubv = glSecondaryColor3ubvEXT
	.globl glSecondaryColor3ui ; .type glSecondaryColor3ui,#function ; glSecondaryColor3ui = glSecondaryColor3uiEXT
	.globl glSecondaryColor3uiv ; .type glSecondaryColor3uiv,#function ; glSecondaryColor3uiv = glSecondaryColor3uivEXT
	.globl glSecondaryColor3us ; .type glSecondaryColor3us,#function ; glSecondaryColor3us = glSecondaryColor3usEXT
	.globl glSecondaryColor3usv ; .type glSecondaryColor3usv,#function ; glSecondaryColor3usv = glSecondaryColor3usvEXT
	.globl glSecondaryColorPointer ; .type glSecondaryColorPointer,#function ; glSecondaryColorPointer = glSecondaryColorPointerEXT
	.globl glMultiDrawArrays ; .type glMultiDrawArrays,#function ; glMultiDrawArrays = glMultiDrawArraysEXT
	.globl glMultiDrawElements ; .type glMultiDrawElements,#function ; glMultiDrawElements = glMultiDrawElementsEXT
	.globl glFogCoordPointer ; .type glFogCoordPointer,#function ; glFogCoordPointer = glFogCoordPointerEXT
	.globl glFogCoordd ; .type glFogCoordd,#function ; glFogCoordd = glFogCoorddEXT
	.globl glFogCoorddv ; .type glFogCoorddv,#function ; glFogCoorddv = glFogCoorddvEXT
	.globl glFogCoordf ; .type glFogCoordf,#function ; glFogCoordf = glFogCoordfEXT
	.globl glFogCoordfv ; .type glFogCoordfv,#function ; glFogCoordfv = glFogCoordfvEXT
	.globl glBlendFuncSeparate ; .type glBlendFuncSeparate,#function ; glBlendFuncSeparate = glBlendFuncSeparateEXT
	.globl glWindowPos2d ; .type glWindowPos2d,#function ; glWindowPos2d = glWindowPos2dMESA
	.globl glWindowPos2dARB ; .type glWindowPos2dARB,#function ; glWindowPos2dARB = glWindowPos2dMESA
	.globl glWindowPos2dv ; .type glWindowPos2dv,#function ; glWindowPos2dv = glWindowPos2dvMESA
	.globl glWindowPos2dvARB ; .type glWindowPos2dvARB,#function ; glWindowPos2dvARB = glWindowPos2dvMESA
	.globl glWindowPos2f ; .type glWindowPos2f,#function ; glWindowPos2f = glWindowPos2fMESA
	.globl glWindowPos2fARB ; .type glWindowPos2fARB,#function ; glWindowPos2fARB = glWindowPos2fMESA
	.globl glWindowPos2fv ; .type glWindowPos2fv,#function ; glWindowPos2fv = glWindowPos2fvMESA
	.globl glWindowPos2fvARB ; .type glWindowPos2fvARB,#function ; glWindowPos2fvARB = glWindowPos2fvMESA
	.globl glWindowPos2i ; .type glWindowPos2i,#function ; glWindowPos2i = glWindowPos2iMESA
	.globl glWindowPos2iARB ; .type glWindowPos2iARB,#function ; glWindowPos2iARB = glWindowPos2iMESA
	.globl glWindowPos2iv ; .type glWindowPos2iv,#function ; glWindowPos2iv = glWindowPos2ivMESA
	.globl glWindowPos2ivARB ; .type glWindowPos2ivARB,#function ; glWindowPos2ivARB = glWindowPos2ivMESA
	.globl glWindowPos2s ; .type glWindowPos2s,#function ; glWindowPos2s = glWindowPos2sMESA
	.globl glWindowPos2sARB ; .type glWindowPos2sARB,#function ; glWindowPos2sARB = glWindowPos2sMESA
	.globl glWindowPos2sv ; .type glWindowPos2sv,#function ; glWindowPos2sv = glWindowPos2svMESA
	.globl glWindowPos2svARB ; .type glWindowPos2svARB,#function ; glWindowPos2svARB = glWindowPos2svMESA
	.globl glWindowPos3d ; .type glWindowPos3d,#function ; glWindowPos3d = glWindowPos3dMESA
	.globl glWindowPos3dARB ; .type glWindowPos3dARB,#function ; glWindowPos3dARB = glWindowPos3dMESA
	.globl glWindowPos3dv ; .type glWindowPos3dv,#function ; glWindowPos3dv = glWindowPos3dvMESA
	.globl glWindowPos3dvARB ; .type glWindowPos3dvARB,#function ; glWindowPos3dvARB = glWindowPos3dvMESA
	.globl glWindowPos3f ; .type glWindowPos3f,#function ; glWindowPos3f = glWindowPos3fMESA
	.globl glWindowPos3fARB ; .type glWindowPos3fARB,#function ; glWindowPos3fARB = glWindowPos3fMESA
	.globl glWindowPos3fv ; .type glWindowPos3fv,#function ; glWindowPos3fv = glWindowPos3fvMESA
	.globl glWindowPos3fvARB ; .type glWindowPos3fvARB,#function ; glWindowPos3fvARB = glWindowPos3fvMESA
	.globl glWindowPos3i ; .type glWindowPos3i,#function ; glWindowPos3i = glWindowPos3iMESA
	.globl glWindowPos3iARB ; .type glWindowPos3iARB,#function ; glWindowPos3iARB = glWindowPos3iMESA
	.globl glWindowPos3iv ; .type glWindowPos3iv,#function ; glWindowPos3iv = glWindowPos3ivMESA
	.globl glWindowPos3ivARB ; .type glWindowPos3ivARB,#function ; glWindowPos3ivARB = glWindowPos3ivMESA
	.globl glWindowPos3s ; .type glWindowPos3s,#function ; glWindowPos3s = glWindowPos3sMESA
	.globl glWindowPos3sARB ; .type glWindowPos3sARB,#function ; glWindowPos3sARB = glWindowPos3sMESA
	.globl glWindowPos3sv ; .type glWindowPos3sv,#function ; glWindowPos3sv = glWindowPos3svMESA
	.globl glWindowPos3svARB ; .type glWindowPos3svARB,#function ; glWindowPos3svARB = glWindowPos3svMESA
	.globl glBindProgramARB ; .type glBindProgramARB,#function ; glBindProgramARB = glBindProgramNV
	.globl glDeleteProgramsARB ; .type glDeleteProgramsARB,#function ; glDeleteProgramsARB = glDeleteProgramsNV
	.globl glGenProgramsARB ; .type glGenProgramsARB,#function ; glGenProgramsARB = glGenProgramsNV
	.globl glGetVertexAttribPointerv ; .type glGetVertexAttribPointerv,#function ; glGetVertexAttribPointerv = glGetVertexAttribPointervNV
	.globl glGetVertexAttribPointervARB ; .type glGetVertexAttribPointervARB,#function ; glGetVertexAttribPointervARB = glGetVertexAttribPointervNV
	.globl glIsProgramARB ; .type glIsProgramARB,#function ; glIsProgramARB = glIsProgramNV
	.globl glPointParameteri ; .type glPointParameteri,#function ; glPointParameteri = glPointParameteriNV
	.globl glPointParameteriv ; .type glPointParameteriv,#function ; glPointParameteriv = glPointParameterivNV
	.globl glBlendEquationSeparate ; .type glBlendEquationSeparate,#function ; glBlendEquationSeparate = glBlendEquationSeparateEXT
