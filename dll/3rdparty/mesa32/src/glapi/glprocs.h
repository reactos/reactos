/* DO NOT EDIT - This file generated automatically by gl_procs.py (from Mesa) script */

/*
 * Copyright (C) 1999-2001  Brian Paul   All Rights Reserved.
 * (C) Copyright IBM Corporation 2004, 2006
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


/* This file is only included by glapi.c and is used for
 * the GetProcAddress() function
 */

typedef struct {
    GLint Name_offset;
#if defined(NEED_FUNCTION_POINTER) || defined(GLX_INDIRECT_RENDERING)
    _glapi_proc Address;
#endif
    GLuint Offset;
} glprocs_table_t;

#if   !defined(NEED_FUNCTION_POINTER) && !defined(GLX_INDIRECT_RENDERING)
#  define NAME_FUNC_OFFSET(n,f1,f2,f3,o) { n , o }
#elif  defined(NEED_FUNCTION_POINTER) && !defined(GLX_INDIRECT_RENDERING)
#  define NAME_FUNC_OFFSET(n,f1,f2,f3,o) { n , (_glapi_proc) f1 , o }
#elif  defined(NEED_FUNCTION_POINTER) &&  defined(GLX_INDIRECT_RENDERING)
#  define NAME_FUNC_OFFSET(n,f1,f2,f3,o) { n , (_glapi_proc) f2 , o }
#elif !defined(NEED_FUNCTION_POINTER) &&  defined(GLX_INDIRECT_RENDERING)
#  define NAME_FUNC_OFFSET(n,f1,f2,f3,o) { n , (_glapi_proc) f3 , o }
#endif



static const char gl_string_table[] =
    "glNewList\0"
    "glEndList\0"
    "glCallList\0"
    "glCallLists\0"
    "glDeleteLists\0"
    "glGenLists\0"
    "glListBase\0"
    "glBegin\0"
    "glBitmap\0"
    "glColor3b\0"
    "glColor3bv\0"
    "glColor3d\0"
    "glColor3dv\0"
    "glColor3f\0"
    "glColor3fv\0"
    "glColor3i\0"
    "glColor3iv\0"
    "glColor3s\0"
    "glColor3sv\0"
    "glColor3ub\0"
    "glColor3ubv\0"
    "glColor3ui\0"
    "glColor3uiv\0"
    "glColor3us\0"
    "glColor3usv\0"
    "glColor4b\0"
    "glColor4bv\0"
    "glColor4d\0"
    "glColor4dv\0"
    "glColor4f\0"
    "glColor4fv\0"
    "glColor4i\0"
    "glColor4iv\0"
    "glColor4s\0"
    "glColor4sv\0"
    "glColor4ub\0"
    "glColor4ubv\0"
    "glColor4ui\0"
    "glColor4uiv\0"
    "glColor4us\0"
    "glColor4usv\0"
    "glEdgeFlag\0"
    "glEdgeFlagv\0"
    "glEnd\0"
    "glIndexd\0"
    "glIndexdv\0"
    "glIndexf\0"
    "glIndexfv\0"
    "glIndexi\0"
    "glIndexiv\0"
    "glIndexs\0"
    "glIndexsv\0"
    "glNormal3b\0"
    "glNormal3bv\0"
    "glNormal3d\0"
    "glNormal3dv\0"
    "glNormal3f\0"
    "glNormal3fv\0"
    "glNormal3i\0"
    "glNormal3iv\0"
    "glNormal3s\0"
    "glNormal3sv\0"
    "glRasterPos2d\0"
    "glRasterPos2dv\0"
    "glRasterPos2f\0"
    "glRasterPos2fv\0"
    "glRasterPos2i\0"
    "glRasterPos2iv\0"
    "glRasterPos2s\0"
    "glRasterPos2sv\0"
    "glRasterPos3d\0"
    "glRasterPos3dv\0"
    "glRasterPos3f\0"
    "glRasterPos3fv\0"
    "glRasterPos3i\0"
    "glRasterPos3iv\0"
    "glRasterPos3s\0"
    "glRasterPos3sv\0"
    "glRasterPos4d\0"
    "glRasterPos4dv\0"
    "glRasterPos4f\0"
    "glRasterPos4fv\0"
    "glRasterPos4i\0"
    "glRasterPos4iv\0"
    "glRasterPos4s\0"
    "glRasterPos4sv\0"
    "glRectd\0"
    "glRectdv\0"
    "glRectf\0"
    "glRectfv\0"
    "glRecti\0"
    "glRectiv\0"
    "glRects\0"
    "glRectsv\0"
    "glTexCoord1d\0"
    "glTexCoord1dv\0"
    "glTexCoord1f\0"
    "glTexCoord1fv\0"
    "glTexCoord1i\0"
    "glTexCoord1iv\0"
    "glTexCoord1s\0"
    "glTexCoord1sv\0"
    "glTexCoord2d\0"
    "glTexCoord2dv\0"
    "glTexCoord2f\0"
    "glTexCoord2fv\0"
    "glTexCoord2i\0"
    "glTexCoord2iv\0"
    "glTexCoord2s\0"
    "glTexCoord2sv\0"
    "glTexCoord3d\0"
    "glTexCoord3dv\0"
    "glTexCoord3f\0"
    "glTexCoord3fv\0"
    "glTexCoord3i\0"
    "glTexCoord3iv\0"
    "glTexCoord3s\0"
    "glTexCoord3sv\0"
    "glTexCoord4d\0"
    "glTexCoord4dv\0"
    "glTexCoord4f\0"
    "glTexCoord4fv\0"
    "glTexCoord4i\0"
    "glTexCoord4iv\0"
    "glTexCoord4s\0"
    "glTexCoord4sv\0"
    "glVertex2d\0"
    "glVertex2dv\0"
    "glVertex2f\0"
    "glVertex2fv\0"
    "glVertex2i\0"
    "glVertex2iv\0"
    "glVertex2s\0"
    "glVertex2sv\0"
    "glVertex3d\0"
    "glVertex3dv\0"
    "glVertex3f\0"
    "glVertex3fv\0"
    "glVertex3i\0"
    "glVertex3iv\0"
    "glVertex3s\0"
    "glVertex3sv\0"
    "glVertex4d\0"
    "glVertex4dv\0"
    "glVertex4f\0"
    "glVertex4fv\0"
    "glVertex4i\0"
    "glVertex4iv\0"
    "glVertex4s\0"
    "glVertex4sv\0"
    "glClipPlane\0"
    "glColorMaterial\0"
    "glCullFace\0"
    "glFogf\0"
    "glFogfv\0"
    "glFogi\0"
    "glFogiv\0"
    "glFrontFace\0"
    "glHint\0"
    "glLightf\0"
    "glLightfv\0"
    "glLighti\0"
    "glLightiv\0"
    "glLightModelf\0"
    "glLightModelfv\0"
    "glLightModeli\0"
    "glLightModeliv\0"
    "glLineStipple\0"
    "glLineWidth\0"
    "glMaterialf\0"
    "glMaterialfv\0"
    "glMateriali\0"
    "glMaterialiv\0"
    "glPointSize\0"
    "glPolygonMode\0"
    "glPolygonStipple\0"
    "glScissor\0"
    "glShadeModel\0"
    "glTexParameterf\0"
    "glTexParameterfv\0"
    "glTexParameteri\0"
    "glTexParameteriv\0"
    "glTexImage1D\0"
    "glTexImage2D\0"
    "glTexEnvf\0"
    "glTexEnvfv\0"
    "glTexEnvi\0"
    "glTexEnviv\0"
    "glTexGend\0"
    "glTexGendv\0"
    "glTexGenf\0"
    "glTexGenfv\0"
    "glTexGeni\0"
    "glTexGeniv\0"
    "glFeedbackBuffer\0"
    "glSelectBuffer\0"
    "glRenderMode\0"
    "glInitNames\0"
    "glLoadName\0"
    "glPassThrough\0"
    "glPopName\0"
    "glPushName\0"
    "glDrawBuffer\0"
    "glClear\0"
    "glClearAccum\0"
    "glClearIndex\0"
    "glClearColor\0"
    "glClearStencil\0"
    "glClearDepth\0"
    "glStencilMask\0"
    "glColorMask\0"
    "glDepthMask\0"
    "glIndexMask\0"
    "glAccum\0"
    "glDisable\0"
    "glEnable\0"
    "glFinish\0"
    "glFlush\0"
    "glPopAttrib\0"
    "glPushAttrib\0"
    "glMap1d\0"
    "glMap1f\0"
    "glMap2d\0"
    "glMap2f\0"
    "glMapGrid1d\0"
    "glMapGrid1f\0"
    "glMapGrid2d\0"
    "glMapGrid2f\0"
    "glEvalCoord1d\0"
    "glEvalCoord1dv\0"
    "glEvalCoord1f\0"
    "glEvalCoord1fv\0"
    "glEvalCoord2d\0"
    "glEvalCoord2dv\0"
    "glEvalCoord2f\0"
    "glEvalCoord2fv\0"
    "glEvalMesh1\0"
    "glEvalPoint1\0"
    "glEvalMesh2\0"
    "glEvalPoint2\0"
    "glAlphaFunc\0"
    "glBlendFunc\0"
    "glLogicOp\0"
    "glStencilFunc\0"
    "glStencilOp\0"
    "glDepthFunc\0"
    "glPixelZoom\0"
    "glPixelTransferf\0"
    "glPixelTransferi\0"
    "glPixelStoref\0"
    "glPixelStorei\0"
    "glPixelMapfv\0"
    "glPixelMapuiv\0"
    "glPixelMapusv\0"
    "glReadBuffer\0"
    "glCopyPixels\0"
    "glReadPixels\0"
    "glDrawPixels\0"
    "glGetBooleanv\0"
    "glGetClipPlane\0"
    "glGetDoublev\0"
    "glGetError\0"
    "glGetFloatv\0"
    "glGetIntegerv\0"
    "glGetLightfv\0"
    "glGetLightiv\0"
    "glGetMapdv\0"
    "glGetMapfv\0"
    "glGetMapiv\0"
    "glGetMaterialfv\0"
    "glGetMaterialiv\0"
    "glGetPixelMapfv\0"
    "glGetPixelMapuiv\0"
    "glGetPixelMapusv\0"
    "glGetPolygonStipple\0"
    "glGetString\0"
    "glGetTexEnvfv\0"
    "glGetTexEnviv\0"
    "glGetTexGendv\0"
    "glGetTexGenfv\0"
    "glGetTexGeniv\0"
    "glGetTexImage\0"
    "glGetTexParameterfv\0"
    "glGetTexParameteriv\0"
    "glGetTexLevelParameterfv\0"
    "glGetTexLevelParameteriv\0"
    "glIsEnabled\0"
    "glIsList\0"
    "glDepthRange\0"
    "glFrustum\0"
    "glLoadIdentity\0"
    "glLoadMatrixf\0"
    "glLoadMatrixd\0"
    "glMatrixMode\0"
    "glMultMatrixf\0"
    "glMultMatrixd\0"
    "glOrtho\0"
    "glPopMatrix\0"
    "glPushMatrix\0"
    "glRotated\0"
    "glRotatef\0"
    "glScaled\0"
    "glScalef\0"
    "glTranslated\0"
    "glTranslatef\0"
    "glViewport\0"
    "glArrayElement\0"
    "glBindTexture\0"
    "glColorPointer\0"
    "glDisableClientState\0"
    "glDrawArrays\0"
    "glDrawElements\0"
    "glEdgeFlagPointer\0"
    "glEnableClientState\0"
    "glIndexPointer\0"
    "glIndexub\0"
    "glIndexubv\0"
    "glInterleavedArrays\0"
    "glNormalPointer\0"
    "glPolygonOffset\0"
    "glTexCoordPointer\0"
    "glVertexPointer\0"
    "glAreTexturesResident\0"
    "glCopyTexImage1D\0"
    "glCopyTexImage2D\0"
    "glCopyTexSubImage1D\0"
    "glCopyTexSubImage2D\0"
    "glDeleteTextures\0"
    "glGenTextures\0"
    "glGetPointerv\0"
    "glIsTexture\0"
    "glPrioritizeTextures\0"
    "glTexSubImage1D\0"
    "glTexSubImage2D\0"
    "glPopClientAttrib\0"
    "glPushClientAttrib\0"
    "glBlendColor\0"
    "glBlendEquation\0"
    "glDrawRangeElements\0"
    "glColorTable\0"
    "glColorTableParameterfv\0"
    "glColorTableParameteriv\0"
    "glCopyColorTable\0"
    "glGetColorTable\0"
    "glGetColorTableParameterfv\0"
    "glGetColorTableParameteriv\0"
    "glColorSubTable\0"
    "glCopyColorSubTable\0"
    "glConvolutionFilter1D\0"
    "glConvolutionFilter2D\0"
    "glConvolutionParameterf\0"
    "glConvolutionParameterfv\0"
    "glConvolutionParameteri\0"
    "glConvolutionParameteriv\0"
    "glCopyConvolutionFilter1D\0"
    "glCopyConvolutionFilter2D\0"
    "glGetConvolutionFilter\0"
    "glGetConvolutionParameterfv\0"
    "glGetConvolutionParameteriv\0"
    "glGetSeparableFilter\0"
    "glSeparableFilter2D\0"
    "glGetHistogram\0"
    "glGetHistogramParameterfv\0"
    "glGetHistogramParameteriv\0"
    "glGetMinmax\0"
    "glGetMinmaxParameterfv\0"
    "glGetMinmaxParameteriv\0"
    "glHistogram\0"
    "glMinmax\0"
    "glResetHistogram\0"
    "glResetMinmax\0"
    "glTexImage3D\0"
    "glTexSubImage3D\0"
    "glCopyTexSubImage3D\0"
    "glActiveTextureARB\0"
    "glClientActiveTextureARB\0"
    "glMultiTexCoord1dARB\0"
    "glMultiTexCoord1dvARB\0"
    "glMultiTexCoord1fARB\0"
    "glMultiTexCoord1fvARB\0"
    "glMultiTexCoord1iARB\0"
    "glMultiTexCoord1ivARB\0"
    "glMultiTexCoord1sARB\0"
    "glMultiTexCoord1svARB\0"
    "glMultiTexCoord2dARB\0"
    "glMultiTexCoord2dvARB\0"
    "glMultiTexCoord2fARB\0"
    "glMultiTexCoord2fvARB\0"
    "glMultiTexCoord2iARB\0"
    "glMultiTexCoord2ivARB\0"
    "glMultiTexCoord2sARB\0"
    "glMultiTexCoord2svARB\0"
    "glMultiTexCoord3dARB\0"
    "glMultiTexCoord3dvARB\0"
    "glMultiTexCoord3fARB\0"
    "glMultiTexCoord3fvARB\0"
    "glMultiTexCoord3iARB\0"
    "glMultiTexCoord3ivARB\0"
    "glMultiTexCoord3sARB\0"
    "glMultiTexCoord3svARB\0"
    "glMultiTexCoord4dARB\0"
    "glMultiTexCoord4dvARB\0"
    "glMultiTexCoord4fARB\0"
    "glMultiTexCoord4fvARB\0"
    "glMultiTexCoord4iARB\0"
    "glMultiTexCoord4ivARB\0"
    "glMultiTexCoord4sARB\0"
    "glMultiTexCoord4svARB\0"
    "glAttachShader\0"
    "glCreateProgram\0"
    "glCreateShader\0"
    "glDeleteProgram\0"
    "glDeleteShader\0"
    "glDetachShader\0"
    "glGetAttachedShaders\0"
    "glGetProgramInfoLog\0"
    "glGetProgramiv\0"
    "glGetShaderInfoLog\0"
    "glGetShaderiv\0"
    "glIsProgram\0"
    "glIsShader\0"
    "glStencilFuncSeparate\0"
    "glStencilMaskSeparate\0"
    "glStencilOpSeparate\0"
    "glUniformMatrix2x3fv\0"
    "glUniformMatrix2x4fv\0"
    "glUniformMatrix3x2fv\0"
    "glUniformMatrix3x4fv\0"
    "glUniformMatrix4x2fv\0"
    "glUniformMatrix4x3fv\0"
    "glLoadTransposeMatrixdARB\0"
    "glLoadTransposeMatrixfARB\0"
    "glMultTransposeMatrixdARB\0"
    "glMultTransposeMatrixfARB\0"
    "glSampleCoverageARB\0"
    "glCompressedTexImage1DARB\0"
    "glCompressedTexImage2DARB\0"
    "glCompressedTexImage3DARB\0"
    "glCompressedTexSubImage1DARB\0"
    "glCompressedTexSubImage2DARB\0"
    "glCompressedTexSubImage3DARB\0"
    "glGetCompressedTexImageARB\0"
    "glDisableVertexAttribArrayARB\0"
    "glEnableVertexAttribArrayARB\0"
    "glGetProgramEnvParameterdvARB\0"
    "glGetProgramEnvParameterfvARB\0"
    "glGetProgramLocalParameterdvARB\0"
    "glGetProgramLocalParameterfvARB\0"
    "glGetProgramStringARB\0"
    "glGetProgramivARB\0"
    "glGetVertexAttribdvARB\0"
    "glGetVertexAttribfvARB\0"
    "glGetVertexAttribivARB\0"
    "glProgramEnvParameter4dARB\0"
    "glProgramEnvParameter4dvARB\0"
    "glProgramEnvParameter4fARB\0"
    "glProgramEnvParameter4fvARB\0"
    "glProgramLocalParameter4dARB\0"
    "glProgramLocalParameter4dvARB\0"
    "glProgramLocalParameter4fARB\0"
    "glProgramLocalParameter4fvARB\0"
    "glProgramStringARB\0"
    "glVertexAttrib1dARB\0"
    "glVertexAttrib1dvARB\0"
    "glVertexAttrib1fARB\0"
    "glVertexAttrib1fvARB\0"
    "glVertexAttrib1sARB\0"
    "glVertexAttrib1svARB\0"
    "glVertexAttrib2dARB\0"
    "glVertexAttrib2dvARB\0"
    "glVertexAttrib2fARB\0"
    "glVertexAttrib2fvARB\0"
    "glVertexAttrib2sARB\0"
    "glVertexAttrib2svARB\0"
    "glVertexAttrib3dARB\0"
    "glVertexAttrib3dvARB\0"
    "glVertexAttrib3fARB\0"
    "glVertexAttrib3fvARB\0"
    "glVertexAttrib3sARB\0"
    "glVertexAttrib3svARB\0"
    "glVertexAttrib4NbvARB\0"
    "glVertexAttrib4NivARB\0"
    "glVertexAttrib4NsvARB\0"
    "glVertexAttrib4NubARB\0"
    "glVertexAttrib4NubvARB\0"
    "glVertexAttrib4NuivARB\0"
    "glVertexAttrib4NusvARB\0"
    "glVertexAttrib4bvARB\0"
    "glVertexAttrib4dARB\0"
    "glVertexAttrib4dvARB\0"
    "glVertexAttrib4fARB\0"
    "glVertexAttrib4fvARB\0"
    "glVertexAttrib4ivARB\0"
    "glVertexAttrib4sARB\0"
    "glVertexAttrib4svARB\0"
    "glVertexAttrib4ubvARB\0"
    "glVertexAttrib4uivARB\0"
    "glVertexAttrib4usvARB\0"
    "glVertexAttribPointerARB\0"
    "glBindBufferARB\0"
    "glBufferDataARB\0"
    "glBufferSubDataARB\0"
    "glDeleteBuffersARB\0"
    "glGenBuffersARB\0"
    "glGetBufferParameterivARB\0"
    "glGetBufferPointervARB\0"
    "glGetBufferSubDataARB\0"
    "glIsBufferARB\0"
    "glMapBufferARB\0"
    "glUnmapBufferARB\0"
    "glBeginQueryARB\0"
    "glDeleteQueriesARB\0"
    "glEndQueryARB\0"
    "glGenQueriesARB\0"
    "glGetQueryObjectivARB\0"
    "glGetQueryObjectuivARB\0"
    "glGetQueryivARB\0"
    "glIsQueryARB\0"
    "glAttachObjectARB\0"
    "glCompileShaderARB\0"
    "glCreateProgramObjectARB\0"
    "glCreateShaderObjectARB\0"
    "glDeleteObjectARB\0"
    "glDetachObjectARB\0"
    "glGetActiveUniformARB\0"
    "glGetAttachedObjectsARB\0"
    "glGetHandleARB\0"
    "glGetInfoLogARB\0"
    "glGetObjectParameterfvARB\0"
    "glGetObjectParameterivARB\0"
    "glGetShaderSourceARB\0"
    "glGetUniformLocationARB\0"
    "glGetUniformfvARB\0"
    "glGetUniformivARB\0"
    "glLinkProgramARB\0"
    "glShaderSourceARB\0"
    "glUniform1fARB\0"
    "glUniform1fvARB\0"
    "glUniform1iARB\0"
    "glUniform1ivARB\0"
    "glUniform2fARB\0"
    "glUniform2fvARB\0"
    "glUniform2iARB\0"
    "glUniform2ivARB\0"
    "glUniform3fARB\0"
    "glUniform3fvARB\0"
    "glUniform3iARB\0"
    "glUniform3ivARB\0"
    "glUniform4fARB\0"
    "glUniform4fvARB\0"
    "glUniform4iARB\0"
    "glUniform4ivARB\0"
    "glUniformMatrix2fvARB\0"
    "glUniformMatrix3fvARB\0"
    "glUniformMatrix4fvARB\0"
    "glUseProgramObjectARB\0"
    "glValidateProgramARB\0"
    "glBindAttribLocationARB\0"
    "glGetActiveAttribARB\0"
    "glGetAttribLocationARB\0"
    "glDrawBuffersARB\0"
    "glPolygonOffsetEXT\0"
    "glGetPixelTexGenParameterfvSGIS\0"
    "glGetPixelTexGenParameterivSGIS\0"
    "glPixelTexGenParameterfSGIS\0"
    "glPixelTexGenParameterfvSGIS\0"
    "glPixelTexGenParameteriSGIS\0"
    "glPixelTexGenParameterivSGIS\0"
    "glSampleMaskSGIS\0"
    "glSamplePatternSGIS\0"
    "glColorPointerEXT\0"
    "glEdgeFlagPointerEXT\0"
    "glIndexPointerEXT\0"
    "glNormalPointerEXT\0"
    "glTexCoordPointerEXT\0"
    "glVertexPointerEXT\0"
    "glPointParameterfEXT\0"
    "glPointParameterfvEXT\0"
    "glLockArraysEXT\0"
    "glUnlockArraysEXT\0"
    "glCullParameterdvEXT\0"
    "glCullParameterfvEXT\0"
    "glSecondaryColor3bEXT\0"
    "glSecondaryColor3bvEXT\0"
    "glSecondaryColor3dEXT\0"
    "glSecondaryColor3dvEXT\0"
    "glSecondaryColor3fEXT\0"
    "glSecondaryColor3fvEXT\0"
    "glSecondaryColor3iEXT\0"
    "glSecondaryColor3ivEXT\0"
    "glSecondaryColor3sEXT\0"
    "glSecondaryColor3svEXT\0"
    "glSecondaryColor3ubEXT\0"
    "glSecondaryColor3ubvEXT\0"
    "glSecondaryColor3uiEXT\0"
    "glSecondaryColor3uivEXT\0"
    "glSecondaryColor3usEXT\0"
    "glSecondaryColor3usvEXT\0"
    "glSecondaryColorPointerEXT\0"
    "glMultiDrawArraysEXT\0"
    "glMultiDrawElementsEXT\0"
    "glFogCoordPointerEXT\0"
    "glFogCoorddEXT\0"
    "glFogCoorddvEXT\0"
    "glFogCoordfEXT\0"
    "glFogCoordfvEXT\0"
    "glPixelTexGenSGIX\0"
    "glBlendFuncSeparateEXT\0"
    "glFlushVertexArrayRangeNV\0"
    "glVertexArrayRangeNV\0"
    "glCombinerInputNV\0"
    "glCombinerOutputNV\0"
    "glCombinerParameterfNV\0"
    "glCombinerParameterfvNV\0"
    "glCombinerParameteriNV\0"
    "glCombinerParameterivNV\0"
    "glFinalCombinerInputNV\0"
    "glGetCombinerInputParameterfvNV\0"
    "glGetCombinerInputParameterivNV\0"
    "glGetCombinerOutputParameterfvNV\0"
    "glGetCombinerOutputParameterivNV\0"
    "glGetFinalCombinerInputParameterfvNV\0"
    "glGetFinalCombinerInputParameterivNV\0"
    "glResizeBuffersMESA\0"
    "glWindowPos2dMESA\0"
    "glWindowPos2dvMESA\0"
    "glWindowPos2fMESA\0"
    "glWindowPos2fvMESA\0"
    "glWindowPos2iMESA\0"
    "glWindowPos2ivMESA\0"
    "glWindowPos2sMESA\0"
    "glWindowPos2svMESA\0"
    "glWindowPos3dMESA\0"
    "glWindowPos3dvMESA\0"
    "glWindowPos3fMESA\0"
    "glWindowPos3fvMESA\0"
    "glWindowPos3iMESA\0"
    "glWindowPos3ivMESA\0"
    "glWindowPos3sMESA\0"
    "glWindowPos3svMESA\0"
    "glWindowPos4dMESA\0"
    "glWindowPos4dvMESA\0"
    "glWindowPos4fMESA\0"
    "glWindowPos4fvMESA\0"
    "glWindowPos4iMESA\0"
    "glWindowPos4ivMESA\0"
    "glWindowPos4sMESA\0"
    "glWindowPos4svMESA\0"
    "glMultiModeDrawArraysIBM\0"
    "glMultiModeDrawElementsIBM\0"
    "glDeleteFencesNV\0"
    "glFinishFenceNV\0"
    "glGenFencesNV\0"
    "glGetFenceivNV\0"
    "glIsFenceNV\0"
    "glSetFenceNV\0"
    "glTestFenceNV\0"
    "glAreProgramsResidentNV\0"
    "glBindProgramNV\0"
    "glDeleteProgramsNV\0"
    "glExecuteProgramNV\0"
    "glGenProgramsNV\0"
    "glGetProgramParameterdvNV\0"
    "glGetProgramParameterfvNV\0"
    "glGetProgramStringNV\0"
    "glGetProgramivNV\0"
    "glGetTrackMatrixivNV\0"
    "glGetVertexAttribPointervNV\0"
    "glGetVertexAttribdvNV\0"
    "glGetVertexAttribfvNV\0"
    "glGetVertexAttribivNV\0"
    "glIsProgramNV\0"
    "glLoadProgramNV\0"
    "glProgramParameters4dvNV\0"
    "glProgramParameters4fvNV\0"
    "glRequestResidentProgramsNV\0"
    "glTrackMatrixNV\0"
    "glVertexAttrib1dNV\0"
    "glVertexAttrib1dvNV\0"
    "glVertexAttrib1fNV\0"
    "glVertexAttrib1fvNV\0"
    "glVertexAttrib1sNV\0"
    "glVertexAttrib1svNV\0"
    "glVertexAttrib2dNV\0"
    "glVertexAttrib2dvNV\0"
    "glVertexAttrib2fNV\0"
    "glVertexAttrib2fvNV\0"
    "glVertexAttrib2sNV\0"
    "glVertexAttrib2svNV\0"
    "glVertexAttrib3dNV\0"
    "glVertexAttrib3dvNV\0"
    "glVertexAttrib3fNV\0"
    "glVertexAttrib3fvNV\0"
    "glVertexAttrib3sNV\0"
    "glVertexAttrib3svNV\0"
    "glVertexAttrib4dNV\0"
    "glVertexAttrib4dvNV\0"
    "glVertexAttrib4fNV\0"
    "glVertexAttrib4fvNV\0"
    "glVertexAttrib4sNV\0"
    "glVertexAttrib4svNV\0"
    "glVertexAttrib4ubNV\0"
    "glVertexAttrib4ubvNV\0"
    "glVertexAttribPointerNV\0"
    "glVertexAttribs1dvNV\0"
    "glVertexAttribs1fvNV\0"
    "glVertexAttribs1svNV\0"
    "glVertexAttribs2dvNV\0"
    "glVertexAttribs2fvNV\0"
    "glVertexAttribs2svNV\0"
    "glVertexAttribs3dvNV\0"
    "glVertexAttribs3fvNV\0"
    "glVertexAttribs3svNV\0"
    "glVertexAttribs4dvNV\0"
    "glVertexAttribs4fvNV\0"
    "glVertexAttribs4svNV\0"
    "glVertexAttribs4ubvNV\0"
    "glAlphaFragmentOp1ATI\0"
    "glAlphaFragmentOp2ATI\0"
    "glAlphaFragmentOp3ATI\0"
    "glBeginFragmentShaderATI\0"
    "glBindFragmentShaderATI\0"
    "glColorFragmentOp1ATI\0"
    "glColorFragmentOp2ATI\0"
    "glColorFragmentOp3ATI\0"
    "glDeleteFragmentShaderATI\0"
    "glEndFragmentShaderATI\0"
    "glGenFragmentShadersATI\0"
    "glPassTexCoordATI\0"
    "glSampleMapATI\0"
    "glSetFragmentShaderConstantATI\0"
    "glPointParameteriNV\0"
    "glPointParameterivNV\0"
    "glActiveStencilFaceEXT\0"
    "glBindVertexArrayAPPLE\0"
    "glDeleteVertexArraysAPPLE\0"
    "glGenVertexArraysAPPLE\0"
    "glIsVertexArrayAPPLE\0"
    "glGetProgramNamedParameterdvNV\0"
    "glGetProgramNamedParameterfvNV\0"
    "glProgramNamedParameter4dNV\0"
    "glProgramNamedParameter4dvNV\0"
    "glProgramNamedParameter4fNV\0"
    "glProgramNamedParameter4fvNV\0"
    "glDepthBoundsEXT\0"
    "glBlendEquationSeparateEXT\0"
    "glBindFramebufferEXT\0"
    "glBindRenderbufferEXT\0"
    "glCheckFramebufferStatusEXT\0"
    "glDeleteFramebuffersEXT\0"
    "glDeleteRenderbuffersEXT\0"
    "glFramebufferRenderbufferEXT\0"
    "glFramebufferTexture1DEXT\0"
    "glFramebufferTexture2DEXT\0"
    "glFramebufferTexture3DEXT\0"
    "glGenFramebuffersEXT\0"
    "glGenRenderbuffersEXT\0"
    "glGenerateMipmapEXT\0"
    "glGetFramebufferAttachmentParameterivEXT\0"
    "glGetRenderbufferParameterivEXT\0"
    "glIsFramebufferEXT\0"
    "glIsRenderbufferEXT\0"
    "glRenderbufferStorageEXT\0"
    "glBlitFramebufferEXT\0"
    "glFramebufferTextureLayerEXT\0"
    "glStencilFuncSeparateATI\0"
    "glProgramEnvParameters4fvEXT\0"
    "glProgramLocalParameters4fvEXT\0"
    "glGetQueryObjecti64vEXT\0"
    "glGetQueryObjectui64vEXT\0"
    "glArrayElementEXT\0"
    "glBindTextureEXT\0"
    "glDrawArraysEXT\0"
    "glAreTexturesResidentEXT\0"
    "glCopyTexImage1DEXT\0"
    "glCopyTexImage2DEXT\0"
    "glCopyTexSubImage1DEXT\0"
    "glCopyTexSubImage2DEXT\0"
    "glDeleteTexturesEXT\0"
    "glGenTexturesEXT\0"
    "glGetPointervEXT\0"
    "glIsTextureEXT\0"
    "glPrioritizeTexturesEXT\0"
    "glTexSubImage1DEXT\0"
    "glTexSubImage2DEXT\0"
    "glBlendColorEXT\0"
    "glBlendEquationEXT\0"
    "glDrawRangeElementsEXT\0"
    "glColorTableSGI\0"
    "glColorTableEXT\0"
    "glColorTableParameterfvSGI\0"
    "glColorTableParameterivSGI\0"
    "glCopyColorTableSGI\0"
    "glGetColorTableSGI\0"
    "glGetColorTableEXT\0"
    "glGetColorTableParameterfvSGI\0"
    "glGetColorTableParameterfvEXT\0"
    "glGetColorTableParameterivSGI\0"
    "glGetColorTableParameterivEXT\0"
    "glColorSubTableEXT\0"
    "glCopyColorSubTableEXT\0"
    "glConvolutionFilter1DEXT\0"
    "glConvolutionFilter2DEXT\0"
    "glConvolutionParameterfEXT\0"
    "glConvolutionParameterfvEXT\0"
    "glConvolutionParameteriEXT\0"
    "glConvolutionParameterivEXT\0"
    "glCopyConvolutionFilter1DEXT\0"
    "glCopyConvolutionFilter2DEXT\0"
    "glGetConvolutionFilterEXT\0"
    "glGetConvolutionParameterfvEXT\0"
    "glGetConvolutionParameterivEXT\0"
    "glGetSeparableFilterEXT\0"
    "glSeparableFilter2DEXT\0"
    "glGetHistogramEXT\0"
    "glGetHistogramParameterfvEXT\0"
    "glGetHistogramParameterivEXT\0"
    "glGetMinmaxEXT\0"
    "glGetMinmaxParameterfvEXT\0"
    "glGetMinmaxParameterivEXT\0"
    "glHistogramEXT\0"
    "glMinmaxEXT\0"
    "glResetHistogramEXT\0"
    "glResetMinmaxEXT\0"
    "glTexImage3DEXT\0"
    "glTexSubImage3DEXT\0"
    "glCopyTexSubImage3DEXT\0"
    "glActiveTexture\0"
    "glClientActiveTexture\0"
    "glMultiTexCoord1d\0"
    "glMultiTexCoord1dv\0"
    "glMultiTexCoord1f\0"
    "glMultiTexCoord1fv\0"
    "glMultiTexCoord1i\0"
    "glMultiTexCoord1iv\0"
    "glMultiTexCoord1s\0"
    "glMultiTexCoord1sv\0"
    "glMultiTexCoord2d\0"
    "glMultiTexCoord2dv\0"
    "glMultiTexCoord2f\0"
    "glMultiTexCoord2fv\0"
    "glMultiTexCoord2i\0"
    "glMultiTexCoord2iv\0"
    "glMultiTexCoord2s\0"
    "glMultiTexCoord2sv\0"
    "glMultiTexCoord3d\0"
    "glMultiTexCoord3dv\0"
    "glMultiTexCoord3f\0"
    "glMultiTexCoord3fv\0"
    "glMultiTexCoord3i\0"
    "glMultiTexCoord3iv\0"
    "glMultiTexCoord3s\0"
    "glMultiTexCoord3sv\0"
    "glMultiTexCoord4d\0"
    "glMultiTexCoord4dv\0"
    "glMultiTexCoord4f\0"
    "glMultiTexCoord4fv\0"
    "glMultiTexCoord4i\0"
    "glMultiTexCoord4iv\0"
    "glMultiTexCoord4s\0"
    "glMultiTexCoord4sv\0"
    "glStencilOpSeparateATI\0"
    "glLoadTransposeMatrixd\0"
    "glLoadTransposeMatrixf\0"
    "glMultTransposeMatrixd\0"
    "glMultTransposeMatrixf\0"
    "glSampleCoverage\0"
    "glCompressedTexImage1D\0"
    "glCompressedTexImage2D\0"
    "glCompressedTexImage3D\0"
    "glCompressedTexSubImage1D\0"
    "glCompressedTexSubImage2D\0"
    "glCompressedTexSubImage3D\0"
    "glGetCompressedTexImage\0"
    "glDisableVertexAttribArray\0"
    "glEnableVertexAttribArray\0"
    "glGetVertexAttribdv\0"
    "glGetVertexAttribfv\0"
    "glGetVertexAttribiv\0"
    "glProgramParameter4dNV\0"
    "glProgramParameter4dvNV\0"
    "glProgramParameter4fNV\0"
    "glProgramParameter4fvNV\0"
    "glVertexAttrib1d\0"
    "glVertexAttrib1dv\0"
    "glVertexAttrib1f\0"
    "glVertexAttrib1fv\0"
    "glVertexAttrib1s\0"
    "glVertexAttrib1sv\0"
    "glVertexAttrib2d\0"
    "glVertexAttrib2dv\0"
    "glVertexAttrib2f\0"
    "glVertexAttrib2fv\0"
    "glVertexAttrib2s\0"
    "glVertexAttrib2sv\0"
    "glVertexAttrib3d\0"
    "glVertexAttrib3dv\0"
    "glVertexAttrib3f\0"
    "glVertexAttrib3fv\0"
    "glVertexAttrib3s\0"
    "glVertexAttrib3sv\0"
    "glVertexAttrib4Nbv\0"
    "glVertexAttrib4Niv\0"
    "glVertexAttrib4Nsv\0"
    "glVertexAttrib4Nub\0"
    "glVertexAttrib4Nubv\0"
    "glVertexAttrib4Nuiv\0"
    "glVertexAttrib4Nusv\0"
    "glVertexAttrib4bv\0"
    "glVertexAttrib4d\0"
    "glVertexAttrib4dv\0"
    "glVertexAttrib4f\0"
    "glVertexAttrib4fv\0"
    "glVertexAttrib4iv\0"
    "glVertexAttrib4s\0"
    "glVertexAttrib4sv\0"
    "glVertexAttrib4ubv\0"
    "glVertexAttrib4uiv\0"
    "glVertexAttrib4usv\0"
    "glVertexAttribPointer\0"
    "glBindBuffer\0"
    "glBufferData\0"
    "glBufferSubData\0"
    "glDeleteBuffers\0"
    "glGenBuffers\0"
    "glGetBufferParameteriv\0"
    "glGetBufferPointerv\0"
    "glGetBufferSubData\0"
    "glIsBuffer\0"
    "glMapBuffer\0"
    "glUnmapBuffer\0"
    "glBeginQuery\0"
    "glDeleteQueries\0"
    "glEndQuery\0"
    "glGenQueries\0"
    "glGetQueryObjectiv\0"
    "glGetQueryObjectuiv\0"
    "glGetQueryiv\0"
    "glIsQuery\0"
    "glCompileShader\0"
    "glGetActiveUniform\0"
    "glGetShaderSource\0"
    "glGetUniformLocation\0"
    "glGetUniformfv\0"
    "glGetUniformiv\0"
    "glLinkProgram\0"
    "glShaderSource\0"
    "glUniform1f\0"
    "glUniform1fv\0"
    "glUniform1i\0"
    "glUniform1iv\0"
    "glUniform2f\0"
    "glUniform2fv\0"
    "glUniform2i\0"
    "glUniform2iv\0"
    "glUniform3f\0"
    "glUniform3fv\0"
    "glUniform3i\0"
    "glUniform3iv\0"
    "glUniform4f\0"
    "glUniform4fv\0"
    "glUniform4i\0"
    "glUniform4iv\0"
    "glUniformMatrix2fv\0"
    "glUniformMatrix3fv\0"
    "glUniformMatrix4fv\0"
    "glUseProgram\0"
    "glValidateProgram\0"
    "glBindAttribLocation\0"
    "glGetActiveAttrib\0"
    "glGetAttribLocation\0"
    "glDrawBuffers\0"
    "glDrawBuffersATI\0"
    "glSampleMaskEXT\0"
    "glSamplePatternEXT\0"
    "glPointParameterf\0"
    "glPointParameterfARB\0"
    "glPointParameterfSGIS\0"
    "glPointParameterfv\0"
    "glPointParameterfvARB\0"
    "glPointParameterfvSGIS\0"
    "glSecondaryColor3b\0"
    "glSecondaryColor3bv\0"
    "glSecondaryColor3d\0"
    "glSecondaryColor3dv\0"
    "glSecondaryColor3f\0"
    "glSecondaryColor3fv\0"
    "glSecondaryColor3i\0"
    "glSecondaryColor3iv\0"
    "glSecondaryColor3s\0"
    "glSecondaryColor3sv\0"
    "glSecondaryColor3ub\0"
    "glSecondaryColor3ubv\0"
    "glSecondaryColor3ui\0"
    "glSecondaryColor3uiv\0"
    "glSecondaryColor3us\0"
    "glSecondaryColor3usv\0"
    "glSecondaryColorPointer\0"
    "glMultiDrawArrays\0"
    "glMultiDrawElements\0"
    "glFogCoordPointer\0"
    "glFogCoordd\0"
    "glFogCoorddv\0"
    "glFogCoordf\0"
    "glFogCoordfv\0"
    "glBlendFuncSeparate\0"
    "glBlendFuncSeparateINGR\0"
    "glWindowPos2d\0"
    "glWindowPos2dARB\0"
    "glWindowPos2dv\0"
    "glWindowPos2dvARB\0"
    "glWindowPos2f\0"
    "glWindowPos2fARB\0"
    "glWindowPos2fv\0"
    "glWindowPos2fvARB\0"
    "glWindowPos2i\0"
    "glWindowPos2iARB\0"
    "glWindowPos2iv\0"
    "glWindowPos2ivARB\0"
    "glWindowPos2s\0"
    "glWindowPos2sARB\0"
    "glWindowPos2sv\0"
    "glWindowPos2svARB\0"
    "glWindowPos3d\0"
    "glWindowPos3dARB\0"
    "glWindowPos3dv\0"
    "glWindowPos3dvARB\0"
    "glWindowPos3f\0"
    "glWindowPos3fARB\0"
    "glWindowPos3fv\0"
    "glWindowPos3fvARB\0"
    "glWindowPos3i\0"
    "glWindowPos3iARB\0"
    "glWindowPos3iv\0"
    "glWindowPos3ivARB\0"
    "glWindowPos3s\0"
    "glWindowPos3sARB\0"
    "glWindowPos3sv\0"
    "glWindowPos3svARB\0"
    "glBindProgramARB\0"
    "glDeleteProgramsARB\0"
    "glGenProgramsARB\0"
    "glGetVertexAttribPointerv\0"
    "glGetVertexAttribPointervARB\0"
    "glIsProgramARB\0"
    "glPointParameteri\0"
    "glPointParameteriv\0"
    "glBlendEquationSeparate\0"
    "glBlendEquationSeparateATI\0"
    ;


#ifdef USE_MGL_NAMESPACE
#define gl_dispatch_stub_343 mgl_dispatch_stub_343
#define gl_dispatch_stub_344 mgl_dispatch_stub_344
#define gl_dispatch_stub_345 mgl_dispatch_stub_345
#define gl_dispatch_stub_356 mgl_dispatch_stub_356
#define gl_dispatch_stub_357 mgl_dispatch_stub_357
#define gl_dispatch_stub_358 mgl_dispatch_stub_358
#define gl_dispatch_stub_359 mgl_dispatch_stub_359
#define gl_dispatch_stub_361 mgl_dispatch_stub_361
#define gl_dispatch_stub_362 mgl_dispatch_stub_362
#define gl_dispatch_stub_363 mgl_dispatch_stub_363
#define gl_dispatch_stub_364 mgl_dispatch_stub_364
#define gl_dispatch_stub_365 mgl_dispatch_stub_365
#define gl_dispatch_stub_366 mgl_dispatch_stub_366
#define gl_dispatch_stub_562 mgl_dispatch_stub_562
#define gl_dispatch_stub_563 mgl_dispatch_stub_563
#define gl_dispatch_stub_564 mgl_dispatch_stub_564
#define gl_dispatch_stub_565 mgl_dispatch_stub_565
#define gl_dispatch_stub_566 mgl_dispatch_stub_566
#define gl_dispatch_stub_567 mgl_dispatch_stub_567
#define gl_dispatch_stub_568 mgl_dispatch_stub_568
#define gl_dispatch_stub_569 mgl_dispatch_stub_569
#define gl_dispatch_stub_580 mgl_dispatch_stub_580
#define gl_dispatch_stub_581 mgl_dispatch_stub_581
#define gl_dispatch_stub_606 mgl_dispatch_stub_606
#define gl_dispatch_stub_648 mgl_dispatch_stub_648
#define gl_dispatch_stub_649 mgl_dispatch_stub_649
#define gl_dispatch_stub_650 mgl_dispatch_stub_650
#define gl_dispatch_stub_651 mgl_dispatch_stub_651
#define gl_dispatch_stub_652 mgl_dispatch_stub_652
#define gl_dispatch_stub_653 mgl_dispatch_stub_653
#define gl_dispatch_stub_654 mgl_dispatch_stub_654
#define gl_dispatch_stub_655 mgl_dispatch_stub_655
#define gl_dispatch_stub_656 mgl_dispatch_stub_656
#define gl_dispatch_stub_733 mgl_dispatch_stub_733
#define gl_dispatch_stub_734 mgl_dispatch_stub_734
#define gl_dispatch_stub_735 mgl_dispatch_stub_735
#define gl_dispatch_stub_736 mgl_dispatch_stub_736
#define gl_dispatch_stub_737 mgl_dispatch_stub_737
#define gl_dispatch_stub_744 mgl_dispatch_stub_744
#define gl_dispatch_stub_745 mgl_dispatch_stub_745
#define gl_dispatch_stub_763 mgl_dispatch_stub_763
#define gl_dispatch_stub_765 mgl_dispatch_stub_765
#define gl_dispatch_stub_766 mgl_dispatch_stub_766
#define gl_dispatch_stub_767 mgl_dispatch_stub_767
#define gl_dispatch_stub_768 mgl_dispatch_stub_768
#define gl_dispatch_stub_769 mgl_dispatch_stub_769
#endif /* USE_MGL_NAMESPACE */


/* FIXME: Having these (incorrect) prototypes here is ugly. */
#if defined(NEED_FUNCTION_POINTER) || defined(GLX_INDIRECT_RENDERING)
extern void gl_dispatch_stub_343(void);
extern void gl_dispatch_stub_344(void);
extern void gl_dispatch_stub_345(void);
extern void gl_dispatch_stub_356(void);
extern void gl_dispatch_stub_357(void);
extern void gl_dispatch_stub_358(void);
extern void gl_dispatch_stub_359(void);
extern void gl_dispatch_stub_361(void);
extern void gl_dispatch_stub_362(void);
extern void gl_dispatch_stub_363(void);
extern void gl_dispatch_stub_364(void);
extern void gl_dispatch_stub_365(void);
extern void gl_dispatch_stub_366(void);
extern void gl_dispatch_stub_562(void);
extern void gl_dispatch_stub_563(void);
extern void gl_dispatch_stub_564(void);
extern void gl_dispatch_stub_565(void);
extern void gl_dispatch_stub_566(void);
extern void gl_dispatch_stub_567(void);
extern void gl_dispatch_stub_568(void);
extern void gl_dispatch_stub_569(void);
extern void gl_dispatch_stub_580(void);
extern void gl_dispatch_stub_581(void);
extern void gl_dispatch_stub_606(void);
extern void gl_dispatch_stub_648(void);
extern void gl_dispatch_stub_649(void);
extern void gl_dispatch_stub_650(void);
extern void gl_dispatch_stub_651(void);
extern void gl_dispatch_stub_652(void);
extern void gl_dispatch_stub_653(void);
extern void gl_dispatch_stub_654(void);
extern void gl_dispatch_stub_655(void);
extern void gl_dispatch_stub_656(void);
extern void gl_dispatch_stub_733(void);
extern void gl_dispatch_stub_734(void);
extern void gl_dispatch_stub_735(void);
extern void gl_dispatch_stub_736(void);
extern void gl_dispatch_stub_737(void);
extern void gl_dispatch_stub_744(void);
extern void gl_dispatch_stub_745(void);
extern void gl_dispatch_stub_763(void);
extern void gl_dispatch_stub_765(void);
extern void gl_dispatch_stub_766(void);
extern void gl_dispatch_stub_767(void);
extern void gl_dispatch_stub_768(void);
extern void gl_dispatch_stub_769(void);
#endif /* defined(NEED_FUNCTION_POINTER) || defined(GLX_INDIRECT_RENDERING) */

static const glprocs_table_t static_functions[] = {
    NAME_FUNC_OFFSET(    0, glNewList, glNewList, NULL, _gloffset_NewList),
    NAME_FUNC_OFFSET(   10, glEndList, glEndList, NULL, _gloffset_EndList),
    NAME_FUNC_OFFSET(   20, glCallList, glCallList, NULL, _gloffset_CallList),
    NAME_FUNC_OFFSET(   31, glCallLists, glCallLists, NULL, _gloffset_CallLists),
    NAME_FUNC_OFFSET(   43, glDeleteLists, glDeleteLists, NULL, _gloffset_DeleteLists),
    NAME_FUNC_OFFSET(   57, glGenLists, glGenLists, NULL, _gloffset_GenLists),
    NAME_FUNC_OFFSET(   68, glListBase, glListBase, NULL, _gloffset_ListBase),
    NAME_FUNC_OFFSET(   79, glBegin, glBegin, NULL, _gloffset_Begin),
    NAME_FUNC_OFFSET(   87, glBitmap, glBitmap, NULL, _gloffset_Bitmap),
    NAME_FUNC_OFFSET(   96, glColor3b, glColor3b, NULL, _gloffset_Color3b),
    NAME_FUNC_OFFSET(  106, glColor3bv, glColor3bv, NULL, _gloffset_Color3bv),
    NAME_FUNC_OFFSET(  117, glColor3d, glColor3d, NULL, _gloffset_Color3d),
    NAME_FUNC_OFFSET(  127, glColor3dv, glColor3dv, NULL, _gloffset_Color3dv),
    NAME_FUNC_OFFSET(  138, glColor3f, glColor3f, NULL, _gloffset_Color3f),
    NAME_FUNC_OFFSET(  148, glColor3fv, glColor3fv, NULL, _gloffset_Color3fv),
    NAME_FUNC_OFFSET(  159, glColor3i, glColor3i, NULL, _gloffset_Color3i),
    NAME_FUNC_OFFSET(  169, glColor3iv, glColor3iv, NULL, _gloffset_Color3iv),
    NAME_FUNC_OFFSET(  180, glColor3s, glColor3s, NULL, _gloffset_Color3s),
    NAME_FUNC_OFFSET(  190, glColor3sv, glColor3sv, NULL, _gloffset_Color3sv),
    NAME_FUNC_OFFSET(  201, glColor3ub, glColor3ub, NULL, _gloffset_Color3ub),
    NAME_FUNC_OFFSET(  212, glColor3ubv, glColor3ubv, NULL, _gloffset_Color3ubv),
    NAME_FUNC_OFFSET(  224, glColor3ui, glColor3ui, NULL, _gloffset_Color3ui),
    NAME_FUNC_OFFSET(  235, glColor3uiv, glColor3uiv, NULL, _gloffset_Color3uiv),
    NAME_FUNC_OFFSET(  247, glColor3us, glColor3us, NULL, _gloffset_Color3us),
    NAME_FUNC_OFFSET(  258, glColor3usv, glColor3usv, NULL, _gloffset_Color3usv),
    NAME_FUNC_OFFSET(  270, glColor4b, glColor4b, NULL, _gloffset_Color4b),
    NAME_FUNC_OFFSET(  280, glColor4bv, glColor4bv, NULL, _gloffset_Color4bv),
    NAME_FUNC_OFFSET(  291, glColor4d, glColor4d, NULL, _gloffset_Color4d),
    NAME_FUNC_OFFSET(  301, glColor4dv, glColor4dv, NULL, _gloffset_Color4dv),
    NAME_FUNC_OFFSET(  312, glColor4f, glColor4f, NULL, _gloffset_Color4f),
    NAME_FUNC_OFFSET(  322, glColor4fv, glColor4fv, NULL, _gloffset_Color4fv),
    NAME_FUNC_OFFSET(  333, glColor4i, glColor4i, NULL, _gloffset_Color4i),
    NAME_FUNC_OFFSET(  343, glColor4iv, glColor4iv, NULL, _gloffset_Color4iv),
    NAME_FUNC_OFFSET(  354, glColor4s, glColor4s, NULL, _gloffset_Color4s),
    NAME_FUNC_OFFSET(  364, glColor4sv, glColor4sv, NULL, _gloffset_Color4sv),
    NAME_FUNC_OFFSET(  375, glColor4ub, glColor4ub, NULL, _gloffset_Color4ub),
    NAME_FUNC_OFFSET(  386, glColor4ubv, glColor4ubv, NULL, _gloffset_Color4ubv),
    NAME_FUNC_OFFSET(  398, glColor4ui, glColor4ui, NULL, _gloffset_Color4ui),
    NAME_FUNC_OFFSET(  409, glColor4uiv, glColor4uiv, NULL, _gloffset_Color4uiv),
    NAME_FUNC_OFFSET(  421, glColor4us, glColor4us, NULL, _gloffset_Color4us),
    NAME_FUNC_OFFSET(  432, glColor4usv, glColor4usv, NULL, _gloffset_Color4usv),
    NAME_FUNC_OFFSET(  444, glEdgeFlag, glEdgeFlag, NULL, _gloffset_EdgeFlag),
    NAME_FUNC_OFFSET(  455, glEdgeFlagv, glEdgeFlagv, NULL, _gloffset_EdgeFlagv),
    NAME_FUNC_OFFSET(  467, glEnd, glEnd, NULL, _gloffset_End),
    NAME_FUNC_OFFSET(  473, glIndexd, glIndexd, NULL, _gloffset_Indexd),
    NAME_FUNC_OFFSET(  482, glIndexdv, glIndexdv, NULL, _gloffset_Indexdv),
    NAME_FUNC_OFFSET(  492, glIndexf, glIndexf, NULL, _gloffset_Indexf),
    NAME_FUNC_OFFSET(  501, glIndexfv, glIndexfv, NULL, _gloffset_Indexfv),
    NAME_FUNC_OFFSET(  511, glIndexi, glIndexi, NULL, _gloffset_Indexi),
    NAME_FUNC_OFFSET(  520, glIndexiv, glIndexiv, NULL, _gloffset_Indexiv),
    NAME_FUNC_OFFSET(  530, glIndexs, glIndexs, NULL, _gloffset_Indexs),
    NAME_FUNC_OFFSET(  539, glIndexsv, glIndexsv, NULL, _gloffset_Indexsv),
    NAME_FUNC_OFFSET(  549, glNormal3b, glNormal3b, NULL, _gloffset_Normal3b),
    NAME_FUNC_OFFSET(  560, glNormal3bv, glNormal3bv, NULL, _gloffset_Normal3bv),
    NAME_FUNC_OFFSET(  572, glNormal3d, glNormal3d, NULL, _gloffset_Normal3d),
    NAME_FUNC_OFFSET(  583, glNormal3dv, glNormal3dv, NULL, _gloffset_Normal3dv),
    NAME_FUNC_OFFSET(  595, glNormal3f, glNormal3f, NULL, _gloffset_Normal3f),
    NAME_FUNC_OFFSET(  606, glNormal3fv, glNormal3fv, NULL, _gloffset_Normal3fv),
    NAME_FUNC_OFFSET(  618, glNormal3i, glNormal3i, NULL, _gloffset_Normal3i),
    NAME_FUNC_OFFSET(  629, glNormal3iv, glNormal3iv, NULL, _gloffset_Normal3iv),
    NAME_FUNC_OFFSET(  641, glNormal3s, glNormal3s, NULL, _gloffset_Normal3s),
    NAME_FUNC_OFFSET(  652, glNormal3sv, glNormal3sv, NULL, _gloffset_Normal3sv),
    NAME_FUNC_OFFSET(  664, glRasterPos2d, glRasterPos2d, NULL, _gloffset_RasterPos2d),
    NAME_FUNC_OFFSET(  678, glRasterPos2dv, glRasterPos2dv, NULL, _gloffset_RasterPos2dv),
    NAME_FUNC_OFFSET(  693, glRasterPos2f, glRasterPos2f, NULL, _gloffset_RasterPos2f),
    NAME_FUNC_OFFSET(  707, glRasterPos2fv, glRasterPos2fv, NULL, _gloffset_RasterPos2fv),
    NAME_FUNC_OFFSET(  722, glRasterPos2i, glRasterPos2i, NULL, _gloffset_RasterPos2i),
    NAME_FUNC_OFFSET(  736, glRasterPos2iv, glRasterPos2iv, NULL, _gloffset_RasterPos2iv),
    NAME_FUNC_OFFSET(  751, glRasterPos2s, glRasterPos2s, NULL, _gloffset_RasterPos2s),
    NAME_FUNC_OFFSET(  765, glRasterPos2sv, glRasterPos2sv, NULL, _gloffset_RasterPos2sv),
    NAME_FUNC_OFFSET(  780, glRasterPos3d, glRasterPos3d, NULL, _gloffset_RasterPos3d),
    NAME_FUNC_OFFSET(  794, glRasterPos3dv, glRasterPos3dv, NULL, _gloffset_RasterPos3dv),
    NAME_FUNC_OFFSET(  809, glRasterPos3f, glRasterPos3f, NULL, _gloffset_RasterPos3f),
    NAME_FUNC_OFFSET(  823, glRasterPos3fv, glRasterPos3fv, NULL, _gloffset_RasterPos3fv),
    NAME_FUNC_OFFSET(  838, glRasterPos3i, glRasterPos3i, NULL, _gloffset_RasterPos3i),
    NAME_FUNC_OFFSET(  852, glRasterPos3iv, glRasterPos3iv, NULL, _gloffset_RasterPos3iv),
    NAME_FUNC_OFFSET(  867, glRasterPos3s, glRasterPos3s, NULL, _gloffset_RasterPos3s),
    NAME_FUNC_OFFSET(  881, glRasterPos3sv, glRasterPos3sv, NULL, _gloffset_RasterPos3sv),
    NAME_FUNC_OFFSET(  896, glRasterPos4d, glRasterPos4d, NULL, _gloffset_RasterPos4d),
    NAME_FUNC_OFFSET(  910, glRasterPos4dv, glRasterPos4dv, NULL, _gloffset_RasterPos4dv),
    NAME_FUNC_OFFSET(  925, glRasterPos4f, glRasterPos4f, NULL, _gloffset_RasterPos4f),
    NAME_FUNC_OFFSET(  939, glRasterPos4fv, glRasterPos4fv, NULL, _gloffset_RasterPos4fv),
    NAME_FUNC_OFFSET(  954, glRasterPos4i, glRasterPos4i, NULL, _gloffset_RasterPos4i),
    NAME_FUNC_OFFSET(  968, glRasterPos4iv, glRasterPos4iv, NULL, _gloffset_RasterPos4iv),
    NAME_FUNC_OFFSET(  983, glRasterPos4s, glRasterPos4s, NULL, _gloffset_RasterPos4s),
    NAME_FUNC_OFFSET(  997, glRasterPos4sv, glRasterPos4sv, NULL, _gloffset_RasterPos4sv),
    NAME_FUNC_OFFSET( 1012, glRectd, glRectd, NULL, _gloffset_Rectd),
    NAME_FUNC_OFFSET( 1020, glRectdv, glRectdv, NULL, _gloffset_Rectdv),
    NAME_FUNC_OFFSET( 1029, glRectf, glRectf, NULL, _gloffset_Rectf),
    NAME_FUNC_OFFSET( 1037, glRectfv, glRectfv, NULL, _gloffset_Rectfv),
    NAME_FUNC_OFFSET( 1046, glRecti, glRecti, NULL, _gloffset_Recti),
    NAME_FUNC_OFFSET( 1054, glRectiv, glRectiv, NULL, _gloffset_Rectiv),
    NAME_FUNC_OFFSET( 1063, glRects, glRects, NULL, _gloffset_Rects),
    NAME_FUNC_OFFSET( 1071, glRectsv, glRectsv, NULL, _gloffset_Rectsv),
    NAME_FUNC_OFFSET( 1080, glTexCoord1d, glTexCoord1d, NULL, _gloffset_TexCoord1d),
    NAME_FUNC_OFFSET( 1093, glTexCoord1dv, glTexCoord1dv, NULL, _gloffset_TexCoord1dv),
    NAME_FUNC_OFFSET( 1107, glTexCoord1f, glTexCoord1f, NULL, _gloffset_TexCoord1f),
    NAME_FUNC_OFFSET( 1120, glTexCoord1fv, glTexCoord1fv, NULL, _gloffset_TexCoord1fv),
    NAME_FUNC_OFFSET( 1134, glTexCoord1i, glTexCoord1i, NULL, _gloffset_TexCoord1i),
    NAME_FUNC_OFFSET( 1147, glTexCoord1iv, glTexCoord1iv, NULL, _gloffset_TexCoord1iv),
    NAME_FUNC_OFFSET( 1161, glTexCoord1s, glTexCoord1s, NULL, _gloffset_TexCoord1s),
    NAME_FUNC_OFFSET( 1174, glTexCoord1sv, glTexCoord1sv, NULL, _gloffset_TexCoord1sv),
    NAME_FUNC_OFFSET( 1188, glTexCoord2d, glTexCoord2d, NULL, _gloffset_TexCoord2d),
    NAME_FUNC_OFFSET( 1201, glTexCoord2dv, glTexCoord2dv, NULL, _gloffset_TexCoord2dv),
    NAME_FUNC_OFFSET( 1215, glTexCoord2f, glTexCoord2f, NULL, _gloffset_TexCoord2f),
    NAME_FUNC_OFFSET( 1228, glTexCoord2fv, glTexCoord2fv, NULL, _gloffset_TexCoord2fv),
    NAME_FUNC_OFFSET( 1242, glTexCoord2i, glTexCoord2i, NULL, _gloffset_TexCoord2i),
    NAME_FUNC_OFFSET( 1255, glTexCoord2iv, glTexCoord2iv, NULL, _gloffset_TexCoord2iv),
    NAME_FUNC_OFFSET( 1269, glTexCoord2s, glTexCoord2s, NULL, _gloffset_TexCoord2s),
    NAME_FUNC_OFFSET( 1282, glTexCoord2sv, glTexCoord2sv, NULL, _gloffset_TexCoord2sv),
    NAME_FUNC_OFFSET( 1296, glTexCoord3d, glTexCoord3d, NULL, _gloffset_TexCoord3d),
    NAME_FUNC_OFFSET( 1309, glTexCoord3dv, glTexCoord3dv, NULL, _gloffset_TexCoord3dv),
    NAME_FUNC_OFFSET( 1323, glTexCoord3f, glTexCoord3f, NULL, _gloffset_TexCoord3f),
    NAME_FUNC_OFFSET( 1336, glTexCoord3fv, glTexCoord3fv, NULL, _gloffset_TexCoord3fv),
    NAME_FUNC_OFFSET( 1350, glTexCoord3i, glTexCoord3i, NULL, _gloffset_TexCoord3i),
    NAME_FUNC_OFFSET( 1363, glTexCoord3iv, glTexCoord3iv, NULL, _gloffset_TexCoord3iv),
    NAME_FUNC_OFFSET( 1377, glTexCoord3s, glTexCoord3s, NULL, _gloffset_TexCoord3s),
    NAME_FUNC_OFFSET( 1390, glTexCoord3sv, glTexCoord3sv, NULL, _gloffset_TexCoord3sv),
    NAME_FUNC_OFFSET( 1404, glTexCoord4d, glTexCoord4d, NULL, _gloffset_TexCoord4d),
    NAME_FUNC_OFFSET( 1417, glTexCoord4dv, glTexCoord4dv, NULL, _gloffset_TexCoord4dv),
    NAME_FUNC_OFFSET( 1431, glTexCoord4f, glTexCoord4f, NULL, _gloffset_TexCoord4f),
    NAME_FUNC_OFFSET( 1444, glTexCoord4fv, glTexCoord4fv, NULL, _gloffset_TexCoord4fv),
    NAME_FUNC_OFFSET( 1458, glTexCoord4i, glTexCoord4i, NULL, _gloffset_TexCoord4i),
    NAME_FUNC_OFFSET( 1471, glTexCoord4iv, glTexCoord4iv, NULL, _gloffset_TexCoord4iv),
    NAME_FUNC_OFFSET( 1485, glTexCoord4s, glTexCoord4s, NULL, _gloffset_TexCoord4s),
    NAME_FUNC_OFFSET( 1498, glTexCoord4sv, glTexCoord4sv, NULL, _gloffset_TexCoord4sv),
    NAME_FUNC_OFFSET( 1512, glVertex2d, glVertex2d, NULL, _gloffset_Vertex2d),
    NAME_FUNC_OFFSET( 1523, glVertex2dv, glVertex2dv, NULL, _gloffset_Vertex2dv),
    NAME_FUNC_OFFSET( 1535, glVertex2f, glVertex2f, NULL, _gloffset_Vertex2f),
    NAME_FUNC_OFFSET( 1546, glVertex2fv, glVertex2fv, NULL, _gloffset_Vertex2fv),
    NAME_FUNC_OFFSET( 1558, glVertex2i, glVertex2i, NULL, _gloffset_Vertex2i),
    NAME_FUNC_OFFSET( 1569, glVertex2iv, glVertex2iv, NULL, _gloffset_Vertex2iv),
    NAME_FUNC_OFFSET( 1581, glVertex2s, glVertex2s, NULL, _gloffset_Vertex2s),
    NAME_FUNC_OFFSET( 1592, glVertex2sv, glVertex2sv, NULL, _gloffset_Vertex2sv),
    NAME_FUNC_OFFSET( 1604, glVertex3d, glVertex3d, NULL, _gloffset_Vertex3d),
    NAME_FUNC_OFFSET( 1615, glVertex3dv, glVertex3dv, NULL, _gloffset_Vertex3dv),
    NAME_FUNC_OFFSET( 1627, glVertex3f, glVertex3f, NULL, _gloffset_Vertex3f),
    NAME_FUNC_OFFSET( 1638, glVertex3fv, glVertex3fv, NULL, _gloffset_Vertex3fv),
    NAME_FUNC_OFFSET( 1650, glVertex3i, glVertex3i, NULL, _gloffset_Vertex3i),
    NAME_FUNC_OFFSET( 1661, glVertex3iv, glVertex3iv, NULL, _gloffset_Vertex3iv),
    NAME_FUNC_OFFSET( 1673, glVertex3s, glVertex3s, NULL, _gloffset_Vertex3s),
    NAME_FUNC_OFFSET( 1684, glVertex3sv, glVertex3sv, NULL, _gloffset_Vertex3sv),
    NAME_FUNC_OFFSET( 1696, glVertex4d, glVertex4d, NULL, _gloffset_Vertex4d),
    NAME_FUNC_OFFSET( 1707, glVertex4dv, glVertex4dv, NULL, _gloffset_Vertex4dv),
    NAME_FUNC_OFFSET( 1719, glVertex4f, glVertex4f, NULL, _gloffset_Vertex4f),
    NAME_FUNC_OFFSET( 1730, glVertex4fv, glVertex4fv, NULL, _gloffset_Vertex4fv),
    NAME_FUNC_OFFSET( 1742, glVertex4i, glVertex4i, NULL, _gloffset_Vertex4i),
    NAME_FUNC_OFFSET( 1753, glVertex4iv, glVertex4iv, NULL, _gloffset_Vertex4iv),
    NAME_FUNC_OFFSET( 1765, glVertex4s, glVertex4s, NULL, _gloffset_Vertex4s),
    NAME_FUNC_OFFSET( 1776, glVertex4sv, glVertex4sv, NULL, _gloffset_Vertex4sv),
    NAME_FUNC_OFFSET( 1788, glClipPlane, glClipPlane, NULL, _gloffset_ClipPlane),
    NAME_FUNC_OFFSET( 1800, glColorMaterial, glColorMaterial, NULL, _gloffset_ColorMaterial),
    NAME_FUNC_OFFSET( 1816, glCullFace, glCullFace, NULL, _gloffset_CullFace),
    NAME_FUNC_OFFSET( 1827, glFogf, glFogf, NULL, _gloffset_Fogf),
    NAME_FUNC_OFFSET( 1834, glFogfv, glFogfv, NULL, _gloffset_Fogfv),
    NAME_FUNC_OFFSET( 1842, glFogi, glFogi, NULL, _gloffset_Fogi),
    NAME_FUNC_OFFSET( 1849, glFogiv, glFogiv, NULL, _gloffset_Fogiv),
    NAME_FUNC_OFFSET( 1857, glFrontFace, glFrontFace, NULL, _gloffset_FrontFace),
    NAME_FUNC_OFFSET( 1869, glHint, glHint, NULL, _gloffset_Hint),
    NAME_FUNC_OFFSET( 1876, glLightf, glLightf, NULL, _gloffset_Lightf),
    NAME_FUNC_OFFSET( 1885, glLightfv, glLightfv, NULL, _gloffset_Lightfv),
    NAME_FUNC_OFFSET( 1895, glLighti, glLighti, NULL, _gloffset_Lighti),
    NAME_FUNC_OFFSET( 1904, glLightiv, glLightiv, NULL, _gloffset_Lightiv),
    NAME_FUNC_OFFSET( 1914, glLightModelf, glLightModelf, NULL, _gloffset_LightModelf),
    NAME_FUNC_OFFSET( 1928, glLightModelfv, glLightModelfv, NULL, _gloffset_LightModelfv),
    NAME_FUNC_OFFSET( 1943, glLightModeli, glLightModeli, NULL, _gloffset_LightModeli),
    NAME_FUNC_OFFSET( 1957, glLightModeliv, glLightModeliv, NULL, _gloffset_LightModeliv),
    NAME_FUNC_OFFSET( 1972, glLineStipple, glLineStipple, NULL, _gloffset_LineStipple),
    NAME_FUNC_OFFSET( 1986, glLineWidth, glLineWidth, NULL, _gloffset_LineWidth),
    NAME_FUNC_OFFSET( 1998, glMaterialf, glMaterialf, NULL, _gloffset_Materialf),
    NAME_FUNC_OFFSET( 2010, glMaterialfv, glMaterialfv, NULL, _gloffset_Materialfv),
    NAME_FUNC_OFFSET( 2023, glMateriali, glMateriali, NULL, _gloffset_Materiali),
    NAME_FUNC_OFFSET( 2035, glMaterialiv, glMaterialiv, NULL, _gloffset_Materialiv),
    NAME_FUNC_OFFSET( 2048, glPointSize, glPointSize, NULL, _gloffset_PointSize),
    NAME_FUNC_OFFSET( 2060, glPolygonMode, glPolygonMode, NULL, _gloffset_PolygonMode),
    NAME_FUNC_OFFSET( 2074, glPolygonStipple, glPolygonStipple, NULL, _gloffset_PolygonStipple),
    NAME_FUNC_OFFSET( 2091, glScissor, glScissor, NULL, _gloffset_Scissor),
    NAME_FUNC_OFFSET( 2101, glShadeModel, glShadeModel, NULL, _gloffset_ShadeModel),
    NAME_FUNC_OFFSET( 2114, glTexParameterf, glTexParameterf, NULL, _gloffset_TexParameterf),
    NAME_FUNC_OFFSET( 2130, glTexParameterfv, glTexParameterfv, NULL, _gloffset_TexParameterfv),
    NAME_FUNC_OFFSET( 2147, glTexParameteri, glTexParameteri, NULL, _gloffset_TexParameteri),
    NAME_FUNC_OFFSET( 2163, glTexParameteriv, glTexParameteriv, NULL, _gloffset_TexParameteriv),
    NAME_FUNC_OFFSET( 2180, glTexImage1D, glTexImage1D, NULL, _gloffset_TexImage1D),
    NAME_FUNC_OFFSET( 2193, glTexImage2D, glTexImage2D, NULL, _gloffset_TexImage2D),
    NAME_FUNC_OFFSET( 2206, glTexEnvf, glTexEnvf, NULL, _gloffset_TexEnvf),
    NAME_FUNC_OFFSET( 2216, glTexEnvfv, glTexEnvfv, NULL, _gloffset_TexEnvfv),
    NAME_FUNC_OFFSET( 2227, glTexEnvi, glTexEnvi, NULL, _gloffset_TexEnvi),
    NAME_FUNC_OFFSET( 2237, glTexEnviv, glTexEnviv, NULL, _gloffset_TexEnviv),
    NAME_FUNC_OFFSET( 2248, glTexGend, glTexGend, NULL, _gloffset_TexGend),
    NAME_FUNC_OFFSET( 2258, glTexGendv, glTexGendv, NULL, _gloffset_TexGendv),
    NAME_FUNC_OFFSET( 2269, glTexGenf, glTexGenf, NULL, _gloffset_TexGenf),
    NAME_FUNC_OFFSET( 2279, glTexGenfv, glTexGenfv, NULL, _gloffset_TexGenfv),
    NAME_FUNC_OFFSET( 2290, glTexGeni, glTexGeni, NULL, _gloffset_TexGeni),
    NAME_FUNC_OFFSET( 2300, glTexGeniv, glTexGeniv, NULL, _gloffset_TexGeniv),
    NAME_FUNC_OFFSET( 2311, glFeedbackBuffer, glFeedbackBuffer, NULL, _gloffset_FeedbackBuffer),
    NAME_FUNC_OFFSET( 2328, glSelectBuffer, glSelectBuffer, NULL, _gloffset_SelectBuffer),
    NAME_FUNC_OFFSET( 2343, glRenderMode, glRenderMode, NULL, _gloffset_RenderMode),
    NAME_FUNC_OFFSET( 2356, glInitNames, glInitNames, NULL, _gloffset_InitNames),
    NAME_FUNC_OFFSET( 2368, glLoadName, glLoadName, NULL, _gloffset_LoadName),
    NAME_FUNC_OFFSET( 2379, glPassThrough, glPassThrough, NULL, _gloffset_PassThrough),
    NAME_FUNC_OFFSET( 2393, glPopName, glPopName, NULL, _gloffset_PopName),
    NAME_FUNC_OFFSET( 2403, glPushName, glPushName, NULL, _gloffset_PushName),
    NAME_FUNC_OFFSET( 2414, glDrawBuffer, glDrawBuffer, NULL, _gloffset_DrawBuffer),
    NAME_FUNC_OFFSET( 2427, glClear, glClear, NULL, _gloffset_Clear),
    NAME_FUNC_OFFSET( 2435, glClearAccum, glClearAccum, NULL, _gloffset_ClearAccum),
    NAME_FUNC_OFFSET( 2448, glClearIndex, glClearIndex, NULL, _gloffset_ClearIndex),
    NAME_FUNC_OFFSET( 2461, glClearColor, glClearColor, NULL, _gloffset_ClearColor),
    NAME_FUNC_OFFSET( 2474, glClearStencil, glClearStencil, NULL, _gloffset_ClearStencil),
    NAME_FUNC_OFFSET( 2489, glClearDepth, glClearDepth, NULL, _gloffset_ClearDepth),
    NAME_FUNC_OFFSET( 2502, glStencilMask, glStencilMask, NULL, _gloffset_StencilMask),
    NAME_FUNC_OFFSET( 2516, glColorMask, glColorMask, NULL, _gloffset_ColorMask),
    NAME_FUNC_OFFSET( 2528, glDepthMask, glDepthMask, NULL, _gloffset_DepthMask),
    NAME_FUNC_OFFSET( 2540, glIndexMask, glIndexMask, NULL, _gloffset_IndexMask),
    NAME_FUNC_OFFSET( 2552, glAccum, glAccum, NULL, _gloffset_Accum),
    NAME_FUNC_OFFSET( 2560, glDisable, glDisable, NULL, _gloffset_Disable),
    NAME_FUNC_OFFSET( 2570, glEnable, glEnable, NULL, _gloffset_Enable),
    NAME_FUNC_OFFSET( 2579, glFinish, glFinish, NULL, _gloffset_Finish),
    NAME_FUNC_OFFSET( 2588, glFlush, glFlush, NULL, _gloffset_Flush),
    NAME_FUNC_OFFSET( 2596, glPopAttrib, glPopAttrib, NULL, _gloffset_PopAttrib),
    NAME_FUNC_OFFSET( 2608, glPushAttrib, glPushAttrib, NULL, _gloffset_PushAttrib),
    NAME_FUNC_OFFSET( 2621, glMap1d, glMap1d, NULL, _gloffset_Map1d),
    NAME_FUNC_OFFSET( 2629, glMap1f, glMap1f, NULL, _gloffset_Map1f),
    NAME_FUNC_OFFSET( 2637, glMap2d, glMap2d, NULL, _gloffset_Map2d),
    NAME_FUNC_OFFSET( 2645, glMap2f, glMap2f, NULL, _gloffset_Map2f),
    NAME_FUNC_OFFSET( 2653, glMapGrid1d, glMapGrid1d, NULL, _gloffset_MapGrid1d),
    NAME_FUNC_OFFSET( 2665, glMapGrid1f, glMapGrid1f, NULL, _gloffset_MapGrid1f),
    NAME_FUNC_OFFSET( 2677, glMapGrid2d, glMapGrid2d, NULL, _gloffset_MapGrid2d),
    NAME_FUNC_OFFSET( 2689, glMapGrid2f, glMapGrid2f, NULL, _gloffset_MapGrid2f),
    NAME_FUNC_OFFSET( 2701, glEvalCoord1d, glEvalCoord1d, NULL, _gloffset_EvalCoord1d),
    NAME_FUNC_OFFSET( 2715, glEvalCoord1dv, glEvalCoord1dv, NULL, _gloffset_EvalCoord1dv),
    NAME_FUNC_OFFSET( 2730, glEvalCoord1f, glEvalCoord1f, NULL, _gloffset_EvalCoord1f),
    NAME_FUNC_OFFSET( 2744, glEvalCoord1fv, glEvalCoord1fv, NULL, _gloffset_EvalCoord1fv),
    NAME_FUNC_OFFSET( 2759, glEvalCoord2d, glEvalCoord2d, NULL, _gloffset_EvalCoord2d),
    NAME_FUNC_OFFSET( 2773, glEvalCoord2dv, glEvalCoord2dv, NULL, _gloffset_EvalCoord2dv),
    NAME_FUNC_OFFSET( 2788, glEvalCoord2f, glEvalCoord2f, NULL, _gloffset_EvalCoord2f),
    NAME_FUNC_OFFSET( 2802, glEvalCoord2fv, glEvalCoord2fv, NULL, _gloffset_EvalCoord2fv),
    NAME_FUNC_OFFSET( 2817, glEvalMesh1, glEvalMesh1, NULL, _gloffset_EvalMesh1),
    NAME_FUNC_OFFSET( 2829, glEvalPoint1, glEvalPoint1, NULL, _gloffset_EvalPoint1),
    NAME_FUNC_OFFSET( 2842, glEvalMesh2, glEvalMesh2, NULL, _gloffset_EvalMesh2),
    NAME_FUNC_OFFSET( 2854, glEvalPoint2, glEvalPoint2, NULL, _gloffset_EvalPoint2),
    NAME_FUNC_OFFSET( 2867, glAlphaFunc, glAlphaFunc, NULL, _gloffset_AlphaFunc),
    NAME_FUNC_OFFSET( 2879, glBlendFunc, glBlendFunc, NULL, _gloffset_BlendFunc),
    NAME_FUNC_OFFSET( 2891, glLogicOp, glLogicOp, NULL, _gloffset_LogicOp),
    NAME_FUNC_OFFSET( 2901, glStencilFunc, glStencilFunc, NULL, _gloffset_StencilFunc),
    NAME_FUNC_OFFSET( 2915, glStencilOp, glStencilOp, NULL, _gloffset_StencilOp),
    NAME_FUNC_OFFSET( 2927, glDepthFunc, glDepthFunc, NULL, _gloffset_DepthFunc),
    NAME_FUNC_OFFSET( 2939, glPixelZoom, glPixelZoom, NULL, _gloffset_PixelZoom),
    NAME_FUNC_OFFSET( 2951, glPixelTransferf, glPixelTransferf, NULL, _gloffset_PixelTransferf),
    NAME_FUNC_OFFSET( 2968, glPixelTransferi, glPixelTransferi, NULL, _gloffset_PixelTransferi),
    NAME_FUNC_OFFSET( 2985, glPixelStoref, glPixelStoref, NULL, _gloffset_PixelStoref),
    NAME_FUNC_OFFSET( 2999, glPixelStorei, glPixelStorei, NULL, _gloffset_PixelStorei),
    NAME_FUNC_OFFSET( 3013, glPixelMapfv, glPixelMapfv, NULL, _gloffset_PixelMapfv),
    NAME_FUNC_OFFSET( 3026, glPixelMapuiv, glPixelMapuiv, NULL, _gloffset_PixelMapuiv),
    NAME_FUNC_OFFSET( 3040, glPixelMapusv, glPixelMapusv, NULL, _gloffset_PixelMapusv),
    NAME_FUNC_OFFSET( 3054, glReadBuffer, glReadBuffer, NULL, _gloffset_ReadBuffer),
    NAME_FUNC_OFFSET( 3067, glCopyPixels, glCopyPixels, NULL, _gloffset_CopyPixels),
    NAME_FUNC_OFFSET( 3080, glReadPixels, glReadPixels, NULL, _gloffset_ReadPixels),
    NAME_FUNC_OFFSET( 3093, glDrawPixels, glDrawPixels, NULL, _gloffset_DrawPixels),
    NAME_FUNC_OFFSET( 3106, glGetBooleanv, glGetBooleanv, NULL, _gloffset_GetBooleanv),
    NAME_FUNC_OFFSET( 3120, glGetClipPlane, glGetClipPlane, NULL, _gloffset_GetClipPlane),
    NAME_FUNC_OFFSET( 3135, glGetDoublev, glGetDoublev, NULL, _gloffset_GetDoublev),
    NAME_FUNC_OFFSET( 3148, glGetError, glGetError, NULL, _gloffset_GetError),
    NAME_FUNC_OFFSET( 3159, glGetFloatv, glGetFloatv, NULL, _gloffset_GetFloatv),
    NAME_FUNC_OFFSET( 3171, glGetIntegerv, glGetIntegerv, NULL, _gloffset_GetIntegerv),
    NAME_FUNC_OFFSET( 3185, glGetLightfv, glGetLightfv, NULL, _gloffset_GetLightfv),
    NAME_FUNC_OFFSET( 3198, glGetLightiv, glGetLightiv, NULL, _gloffset_GetLightiv),
    NAME_FUNC_OFFSET( 3211, glGetMapdv, glGetMapdv, NULL, _gloffset_GetMapdv),
    NAME_FUNC_OFFSET( 3222, glGetMapfv, glGetMapfv, NULL, _gloffset_GetMapfv),
    NAME_FUNC_OFFSET( 3233, glGetMapiv, glGetMapiv, NULL, _gloffset_GetMapiv),
    NAME_FUNC_OFFSET( 3244, glGetMaterialfv, glGetMaterialfv, NULL, _gloffset_GetMaterialfv),
    NAME_FUNC_OFFSET( 3260, glGetMaterialiv, glGetMaterialiv, NULL, _gloffset_GetMaterialiv),
    NAME_FUNC_OFFSET( 3276, glGetPixelMapfv, glGetPixelMapfv, NULL, _gloffset_GetPixelMapfv),
    NAME_FUNC_OFFSET( 3292, glGetPixelMapuiv, glGetPixelMapuiv, NULL, _gloffset_GetPixelMapuiv),
    NAME_FUNC_OFFSET( 3309, glGetPixelMapusv, glGetPixelMapusv, NULL, _gloffset_GetPixelMapusv),
    NAME_FUNC_OFFSET( 3326, glGetPolygonStipple, glGetPolygonStipple, NULL, _gloffset_GetPolygonStipple),
    NAME_FUNC_OFFSET( 3346, glGetString, glGetString, NULL, _gloffset_GetString),
    NAME_FUNC_OFFSET( 3358, glGetTexEnvfv, glGetTexEnvfv, NULL, _gloffset_GetTexEnvfv),
    NAME_FUNC_OFFSET( 3372, glGetTexEnviv, glGetTexEnviv, NULL, _gloffset_GetTexEnviv),
    NAME_FUNC_OFFSET( 3386, glGetTexGendv, glGetTexGendv, NULL, _gloffset_GetTexGendv),
    NAME_FUNC_OFFSET( 3400, glGetTexGenfv, glGetTexGenfv, NULL, _gloffset_GetTexGenfv),
    NAME_FUNC_OFFSET( 3414, glGetTexGeniv, glGetTexGeniv, NULL, _gloffset_GetTexGeniv),
    NAME_FUNC_OFFSET( 3428, glGetTexImage, glGetTexImage, NULL, _gloffset_GetTexImage),
    NAME_FUNC_OFFSET( 3442, glGetTexParameterfv, glGetTexParameterfv, NULL, _gloffset_GetTexParameterfv),
    NAME_FUNC_OFFSET( 3462, glGetTexParameteriv, glGetTexParameteriv, NULL, _gloffset_GetTexParameteriv),
    NAME_FUNC_OFFSET( 3482, glGetTexLevelParameterfv, glGetTexLevelParameterfv, NULL, _gloffset_GetTexLevelParameterfv),
    NAME_FUNC_OFFSET( 3507, glGetTexLevelParameteriv, glGetTexLevelParameteriv, NULL, _gloffset_GetTexLevelParameteriv),
    NAME_FUNC_OFFSET( 3532, glIsEnabled, glIsEnabled, NULL, _gloffset_IsEnabled),
    NAME_FUNC_OFFSET( 3544, glIsList, glIsList, NULL, _gloffset_IsList),
    NAME_FUNC_OFFSET( 3553, glDepthRange, glDepthRange, NULL, _gloffset_DepthRange),
    NAME_FUNC_OFFSET( 3566, glFrustum, glFrustum, NULL, _gloffset_Frustum),
    NAME_FUNC_OFFSET( 3576, glLoadIdentity, glLoadIdentity, NULL, _gloffset_LoadIdentity),
    NAME_FUNC_OFFSET( 3591, glLoadMatrixf, glLoadMatrixf, NULL, _gloffset_LoadMatrixf),
    NAME_FUNC_OFFSET( 3605, glLoadMatrixd, glLoadMatrixd, NULL, _gloffset_LoadMatrixd),
    NAME_FUNC_OFFSET( 3619, glMatrixMode, glMatrixMode, NULL, _gloffset_MatrixMode),
    NAME_FUNC_OFFSET( 3632, glMultMatrixf, glMultMatrixf, NULL, _gloffset_MultMatrixf),
    NAME_FUNC_OFFSET( 3646, glMultMatrixd, glMultMatrixd, NULL, _gloffset_MultMatrixd),
    NAME_FUNC_OFFSET( 3660, glOrtho, glOrtho, NULL, _gloffset_Ortho),
    NAME_FUNC_OFFSET( 3668, glPopMatrix, glPopMatrix, NULL, _gloffset_PopMatrix),
    NAME_FUNC_OFFSET( 3680, glPushMatrix, glPushMatrix, NULL, _gloffset_PushMatrix),
    NAME_FUNC_OFFSET( 3693, glRotated, glRotated, NULL, _gloffset_Rotated),
    NAME_FUNC_OFFSET( 3703, glRotatef, glRotatef, NULL, _gloffset_Rotatef),
    NAME_FUNC_OFFSET( 3713, glScaled, glScaled, NULL, _gloffset_Scaled),
    NAME_FUNC_OFFSET( 3722, glScalef, glScalef, NULL, _gloffset_Scalef),
    NAME_FUNC_OFFSET( 3731, glTranslated, glTranslated, NULL, _gloffset_Translated),
    NAME_FUNC_OFFSET( 3744, glTranslatef, glTranslatef, NULL, _gloffset_Translatef),
    NAME_FUNC_OFFSET( 3757, glViewport, glViewport, NULL, _gloffset_Viewport),
    NAME_FUNC_OFFSET( 3768, glArrayElement, glArrayElement, NULL, _gloffset_ArrayElement),
    NAME_FUNC_OFFSET( 3783, glBindTexture, glBindTexture, NULL, _gloffset_BindTexture),
    NAME_FUNC_OFFSET( 3797, glColorPointer, glColorPointer, NULL, _gloffset_ColorPointer),
    NAME_FUNC_OFFSET( 3812, glDisableClientState, glDisableClientState, NULL, _gloffset_DisableClientState),
    NAME_FUNC_OFFSET( 3833, glDrawArrays, glDrawArrays, NULL, _gloffset_DrawArrays),
    NAME_FUNC_OFFSET( 3846, glDrawElements, glDrawElements, NULL, _gloffset_DrawElements),
    NAME_FUNC_OFFSET( 3861, glEdgeFlagPointer, glEdgeFlagPointer, NULL, _gloffset_EdgeFlagPointer),
    NAME_FUNC_OFFSET( 3879, glEnableClientState, glEnableClientState, NULL, _gloffset_EnableClientState),
    NAME_FUNC_OFFSET( 3899, glIndexPointer, glIndexPointer, NULL, _gloffset_IndexPointer),
    NAME_FUNC_OFFSET( 3914, glIndexub, glIndexub, NULL, _gloffset_Indexub),
    NAME_FUNC_OFFSET( 3924, glIndexubv, glIndexubv, NULL, _gloffset_Indexubv),
    NAME_FUNC_OFFSET( 3935, glInterleavedArrays, glInterleavedArrays, NULL, _gloffset_InterleavedArrays),
    NAME_FUNC_OFFSET( 3955, glNormalPointer, glNormalPointer, NULL, _gloffset_NormalPointer),
    NAME_FUNC_OFFSET( 3971, glPolygonOffset, glPolygonOffset, NULL, _gloffset_PolygonOffset),
    NAME_FUNC_OFFSET( 3987, glTexCoordPointer, glTexCoordPointer, NULL, _gloffset_TexCoordPointer),
    NAME_FUNC_OFFSET( 4005, glVertexPointer, glVertexPointer, NULL, _gloffset_VertexPointer),
    NAME_FUNC_OFFSET( 4021, glAreTexturesResident, glAreTexturesResident, NULL, _gloffset_AreTexturesResident),
    NAME_FUNC_OFFSET( 4043, glCopyTexImage1D, glCopyTexImage1D, NULL, _gloffset_CopyTexImage1D),
    NAME_FUNC_OFFSET( 4060, glCopyTexImage2D, glCopyTexImage2D, NULL, _gloffset_CopyTexImage2D),
    NAME_FUNC_OFFSET( 4077, glCopyTexSubImage1D, glCopyTexSubImage1D, NULL, _gloffset_CopyTexSubImage1D),
    NAME_FUNC_OFFSET( 4097, glCopyTexSubImage2D, glCopyTexSubImage2D, NULL, _gloffset_CopyTexSubImage2D),
    NAME_FUNC_OFFSET( 4117, glDeleteTextures, glDeleteTextures, NULL, _gloffset_DeleteTextures),
    NAME_FUNC_OFFSET( 4134, glGenTextures, glGenTextures, NULL, _gloffset_GenTextures),
    NAME_FUNC_OFFSET( 4148, glGetPointerv, glGetPointerv, NULL, _gloffset_GetPointerv),
    NAME_FUNC_OFFSET( 4162, glIsTexture, glIsTexture, NULL, _gloffset_IsTexture),
    NAME_FUNC_OFFSET( 4174, glPrioritizeTextures, glPrioritizeTextures, NULL, _gloffset_PrioritizeTextures),
    NAME_FUNC_OFFSET( 4195, glTexSubImage1D, glTexSubImage1D, NULL, _gloffset_TexSubImage1D),
    NAME_FUNC_OFFSET( 4211, glTexSubImage2D, glTexSubImage2D, NULL, _gloffset_TexSubImage2D),
    NAME_FUNC_OFFSET( 4227, glPopClientAttrib, glPopClientAttrib, NULL, _gloffset_PopClientAttrib),
    NAME_FUNC_OFFSET( 4245, glPushClientAttrib, glPushClientAttrib, NULL, _gloffset_PushClientAttrib),
    NAME_FUNC_OFFSET( 4264, glBlendColor, glBlendColor, NULL, _gloffset_BlendColor),
    NAME_FUNC_OFFSET( 4277, glBlendEquation, glBlendEquation, NULL, _gloffset_BlendEquation),
    NAME_FUNC_OFFSET( 4293, glDrawRangeElements, glDrawRangeElements, NULL, _gloffset_DrawRangeElements),
    NAME_FUNC_OFFSET( 4313, glColorTable, glColorTable, NULL, _gloffset_ColorTable),
    NAME_FUNC_OFFSET( 4326, glColorTableParameterfv, glColorTableParameterfv, NULL, _gloffset_ColorTableParameterfv),
    NAME_FUNC_OFFSET( 4350, glColorTableParameteriv, glColorTableParameteriv, NULL, _gloffset_ColorTableParameteriv),
    NAME_FUNC_OFFSET( 4374, glCopyColorTable, glCopyColorTable, NULL, _gloffset_CopyColorTable),
    NAME_FUNC_OFFSET( 4391, glGetColorTable, glGetColorTable, NULL, _gloffset_GetColorTable),
    NAME_FUNC_OFFSET( 4407, glGetColorTableParameterfv, glGetColorTableParameterfv, NULL, _gloffset_GetColorTableParameterfv),
    NAME_FUNC_OFFSET( 4434, glGetColorTableParameteriv, glGetColorTableParameteriv, NULL, _gloffset_GetColorTableParameteriv),
    NAME_FUNC_OFFSET( 4461, glColorSubTable, glColorSubTable, NULL, _gloffset_ColorSubTable),
    NAME_FUNC_OFFSET( 4477, glCopyColorSubTable, glCopyColorSubTable, NULL, _gloffset_CopyColorSubTable),
    NAME_FUNC_OFFSET( 4497, glConvolutionFilter1D, glConvolutionFilter1D, NULL, _gloffset_ConvolutionFilter1D),
    NAME_FUNC_OFFSET( 4519, glConvolutionFilter2D, glConvolutionFilter2D, NULL, _gloffset_ConvolutionFilter2D),
    NAME_FUNC_OFFSET( 4541, glConvolutionParameterf, glConvolutionParameterf, NULL, _gloffset_ConvolutionParameterf),
    NAME_FUNC_OFFSET( 4565, glConvolutionParameterfv, glConvolutionParameterfv, NULL, _gloffset_ConvolutionParameterfv),
    NAME_FUNC_OFFSET( 4590, glConvolutionParameteri, glConvolutionParameteri, NULL, _gloffset_ConvolutionParameteri),
    NAME_FUNC_OFFSET( 4614, glConvolutionParameteriv, glConvolutionParameteriv, NULL, _gloffset_ConvolutionParameteriv),
    NAME_FUNC_OFFSET( 4639, glCopyConvolutionFilter1D, glCopyConvolutionFilter1D, NULL, _gloffset_CopyConvolutionFilter1D),
    NAME_FUNC_OFFSET( 4665, glCopyConvolutionFilter2D, glCopyConvolutionFilter2D, NULL, _gloffset_CopyConvolutionFilter2D),
    NAME_FUNC_OFFSET( 4691, glGetConvolutionFilter, glGetConvolutionFilter, NULL, _gloffset_GetConvolutionFilter),
    NAME_FUNC_OFFSET( 4714, glGetConvolutionParameterfv, glGetConvolutionParameterfv, NULL, _gloffset_GetConvolutionParameterfv),
    NAME_FUNC_OFFSET( 4742, glGetConvolutionParameteriv, glGetConvolutionParameteriv, NULL, _gloffset_GetConvolutionParameteriv),
    NAME_FUNC_OFFSET( 4770, glGetSeparableFilter, glGetSeparableFilter, NULL, _gloffset_GetSeparableFilter),
    NAME_FUNC_OFFSET( 4791, glSeparableFilter2D, glSeparableFilter2D, NULL, _gloffset_SeparableFilter2D),
    NAME_FUNC_OFFSET( 4811, glGetHistogram, glGetHistogram, NULL, _gloffset_GetHistogram),
    NAME_FUNC_OFFSET( 4826, glGetHistogramParameterfv, glGetHistogramParameterfv, NULL, _gloffset_GetHistogramParameterfv),
    NAME_FUNC_OFFSET( 4852, glGetHistogramParameteriv, glGetHistogramParameteriv, NULL, _gloffset_GetHistogramParameteriv),
    NAME_FUNC_OFFSET( 4878, glGetMinmax, glGetMinmax, NULL, _gloffset_GetMinmax),
    NAME_FUNC_OFFSET( 4890, glGetMinmaxParameterfv, glGetMinmaxParameterfv, NULL, _gloffset_GetMinmaxParameterfv),
    NAME_FUNC_OFFSET( 4913, glGetMinmaxParameteriv, glGetMinmaxParameteriv, NULL, _gloffset_GetMinmaxParameteriv),
    NAME_FUNC_OFFSET( 4936, glHistogram, glHistogram, NULL, _gloffset_Histogram),
    NAME_FUNC_OFFSET( 4948, glMinmax, glMinmax, NULL, _gloffset_Minmax),
    NAME_FUNC_OFFSET( 4957, glResetHistogram, glResetHistogram, NULL, _gloffset_ResetHistogram),
    NAME_FUNC_OFFSET( 4974, glResetMinmax, glResetMinmax, NULL, _gloffset_ResetMinmax),
    NAME_FUNC_OFFSET( 4988, glTexImage3D, glTexImage3D, NULL, _gloffset_TexImage3D),
    NAME_FUNC_OFFSET( 5001, glTexSubImage3D, glTexSubImage3D, NULL, _gloffset_TexSubImage3D),
    NAME_FUNC_OFFSET( 5017, glCopyTexSubImage3D, glCopyTexSubImage3D, NULL, _gloffset_CopyTexSubImage3D),
    NAME_FUNC_OFFSET( 5037, glActiveTextureARB, glActiveTextureARB, NULL, _gloffset_ActiveTextureARB),
    NAME_FUNC_OFFSET( 5056, glClientActiveTextureARB, glClientActiveTextureARB, NULL, _gloffset_ClientActiveTextureARB),
    NAME_FUNC_OFFSET( 5081, glMultiTexCoord1dARB, glMultiTexCoord1dARB, NULL, _gloffset_MultiTexCoord1dARB),
    NAME_FUNC_OFFSET( 5102, glMultiTexCoord1dvARB, glMultiTexCoord1dvARB, NULL, _gloffset_MultiTexCoord1dvARB),
    NAME_FUNC_OFFSET( 5124, glMultiTexCoord1fARB, glMultiTexCoord1fARB, NULL, _gloffset_MultiTexCoord1fARB),
    NAME_FUNC_OFFSET( 5145, glMultiTexCoord1fvARB, glMultiTexCoord1fvARB, NULL, _gloffset_MultiTexCoord1fvARB),
    NAME_FUNC_OFFSET( 5167, glMultiTexCoord1iARB, glMultiTexCoord1iARB, NULL, _gloffset_MultiTexCoord1iARB),
    NAME_FUNC_OFFSET( 5188, glMultiTexCoord1ivARB, glMultiTexCoord1ivARB, NULL, _gloffset_MultiTexCoord1ivARB),
    NAME_FUNC_OFFSET( 5210, glMultiTexCoord1sARB, glMultiTexCoord1sARB, NULL, _gloffset_MultiTexCoord1sARB),
    NAME_FUNC_OFFSET( 5231, glMultiTexCoord1svARB, glMultiTexCoord1svARB, NULL, _gloffset_MultiTexCoord1svARB),
    NAME_FUNC_OFFSET( 5253, glMultiTexCoord2dARB, glMultiTexCoord2dARB, NULL, _gloffset_MultiTexCoord2dARB),
    NAME_FUNC_OFFSET( 5274, glMultiTexCoord2dvARB, glMultiTexCoord2dvARB, NULL, _gloffset_MultiTexCoord2dvARB),
    NAME_FUNC_OFFSET( 5296, glMultiTexCoord2fARB, glMultiTexCoord2fARB, NULL, _gloffset_MultiTexCoord2fARB),
    NAME_FUNC_OFFSET( 5317, glMultiTexCoord2fvARB, glMultiTexCoord2fvARB, NULL, _gloffset_MultiTexCoord2fvARB),
    NAME_FUNC_OFFSET( 5339, glMultiTexCoord2iARB, glMultiTexCoord2iARB, NULL, _gloffset_MultiTexCoord2iARB),
    NAME_FUNC_OFFSET( 5360, glMultiTexCoord2ivARB, glMultiTexCoord2ivARB, NULL, _gloffset_MultiTexCoord2ivARB),
    NAME_FUNC_OFFSET( 5382, glMultiTexCoord2sARB, glMultiTexCoord2sARB, NULL, _gloffset_MultiTexCoord2sARB),
    NAME_FUNC_OFFSET( 5403, glMultiTexCoord2svARB, glMultiTexCoord2svARB, NULL, _gloffset_MultiTexCoord2svARB),
    NAME_FUNC_OFFSET( 5425, glMultiTexCoord3dARB, glMultiTexCoord3dARB, NULL, _gloffset_MultiTexCoord3dARB),
    NAME_FUNC_OFFSET( 5446, glMultiTexCoord3dvARB, glMultiTexCoord3dvARB, NULL, _gloffset_MultiTexCoord3dvARB),
    NAME_FUNC_OFFSET( 5468, glMultiTexCoord3fARB, glMultiTexCoord3fARB, NULL, _gloffset_MultiTexCoord3fARB),
    NAME_FUNC_OFFSET( 5489, glMultiTexCoord3fvARB, glMultiTexCoord3fvARB, NULL, _gloffset_MultiTexCoord3fvARB),
    NAME_FUNC_OFFSET( 5511, glMultiTexCoord3iARB, glMultiTexCoord3iARB, NULL, _gloffset_MultiTexCoord3iARB),
    NAME_FUNC_OFFSET( 5532, glMultiTexCoord3ivARB, glMultiTexCoord3ivARB, NULL, _gloffset_MultiTexCoord3ivARB),
    NAME_FUNC_OFFSET( 5554, glMultiTexCoord3sARB, glMultiTexCoord3sARB, NULL, _gloffset_MultiTexCoord3sARB),
    NAME_FUNC_OFFSET( 5575, glMultiTexCoord3svARB, glMultiTexCoord3svARB, NULL, _gloffset_MultiTexCoord3svARB),
    NAME_FUNC_OFFSET( 5597, glMultiTexCoord4dARB, glMultiTexCoord4dARB, NULL, _gloffset_MultiTexCoord4dARB),
    NAME_FUNC_OFFSET( 5618, glMultiTexCoord4dvARB, glMultiTexCoord4dvARB, NULL, _gloffset_MultiTexCoord4dvARB),
    NAME_FUNC_OFFSET( 5640, glMultiTexCoord4fARB, glMultiTexCoord4fARB, NULL, _gloffset_MultiTexCoord4fARB),
    NAME_FUNC_OFFSET( 5661, glMultiTexCoord4fvARB, glMultiTexCoord4fvARB, NULL, _gloffset_MultiTexCoord4fvARB),
    NAME_FUNC_OFFSET( 5683, glMultiTexCoord4iARB, glMultiTexCoord4iARB, NULL, _gloffset_MultiTexCoord4iARB),
    NAME_FUNC_OFFSET( 5704, glMultiTexCoord4ivARB, glMultiTexCoord4ivARB, NULL, _gloffset_MultiTexCoord4ivARB),
    NAME_FUNC_OFFSET( 5726, glMultiTexCoord4sARB, glMultiTexCoord4sARB, NULL, _gloffset_MultiTexCoord4sARB),
    NAME_FUNC_OFFSET( 5747, glMultiTexCoord4svARB, glMultiTexCoord4svARB, NULL, _gloffset_MultiTexCoord4svARB),
    NAME_FUNC_OFFSET( 5769, glAttachShader, glAttachShader, NULL, _gloffset_AttachShader),
    NAME_FUNC_OFFSET( 5784, glCreateProgram, glCreateProgram, NULL, _gloffset_CreateProgram),
    NAME_FUNC_OFFSET( 5800, glCreateShader, glCreateShader, NULL, _gloffset_CreateShader),
    NAME_FUNC_OFFSET( 5815, glDeleteProgram, glDeleteProgram, NULL, _gloffset_DeleteProgram),
    NAME_FUNC_OFFSET( 5831, glDeleteShader, glDeleteShader, NULL, _gloffset_DeleteShader),
    NAME_FUNC_OFFSET( 5846, glDetachShader, glDetachShader, NULL, _gloffset_DetachShader),
    NAME_FUNC_OFFSET( 5861, glGetAttachedShaders, glGetAttachedShaders, NULL, _gloffset_GetAttachedShaders),
    NAME_FUNC_OFFSET( 5882, glGetProgramInfoLog, glGetProgramInfoLog, NULL, _gloffset_GetProgramInfoLog),
    NAME_FUNC_OFFSET( 5902, glGetProgramiv, glGetProgramiv, NULL, _gloffset_GetProgramiv),
    NAME_FUNC_OFFSET( 5917, glGetShaderInfoLog, glGetShaderInfoLog, NULL, _gloffset_GetShaderInfoLog),
    NAME_FUNC_OFFSET( 5936, glGetShaderiv, glGetShaderiv, NULL, _gloffset_GetShaderiv),
    NAME_FUNC_OFFSET( 5950, glIsProgram, glIsProgram, NULL, _gloffset_IsProgram),
    NAME_FUNC_OFFSET( 5962, glIsShader, glIsShader, NULL, _gloffset_IsShader),
    NAME_FUNC_OFFSET( 5973, glStencilFuncSeparate, glStencilFuncSeparate, NULL, _gloffset_StencilFuncSeparate),
    NAME_FUNC_OFFSET( 5995, glStencilMaskSeparate, glStencilMaskSeparate, NULL, _gloffset_StencilMaskSeparate),
    NAME_FUNC_OFFSET( 6017, glStencilOpSeparate, glStencilOpSeparate, NULL, _gloffset_StencilOpSeparate),
    NAME_FUNC_OFFSET( 6037, glUniformMatrix2x3fv, glUniformMatrix2x3fv, NULL, _gloffset_UniformMatrix2x3fv),
    NAME_FUNC_OFFSET( 6058, glUniformMatrix2x4fv, glUniformMatrix2x4fv, NULL, _gloffset_UniformMatrix2x4fv),
    NAME_FUNC_OFFSET( 6079, glUniformMatrix3x2fv, glUniformMatrix3x2fv, NULL, _gloffset_UniformMatrix3x2fv),
    NAME_FUNC_OFFSET( 6100, glUniformMatrix3x4fv, glUniformMatrix3x4fv, NULL, _gloffset_UniformMatrix3x4fv),
    NAME_FUNC_OFFSET( 6121, glUniformMatrix4x2fv, glUniformMatrix4x2fv, NULL, _gloffset_UniformMatrix4x2fv),
    NAME_FUNC_OFFSET( 6142, glUniformMatrix4x3fv, glUniformMatrix4x3fv, NULL, _gloffset_UniformMatrix4x3fv),
    NAME_FUNC_OFFSET( 6163, glLoadTransposeMatrixdARB, glLoadTransposeMatrixdARB, NULL, _gloffset_LoadTransposeMatrixdARB),
    NAME_FUNC_OFFSET( 6189, glLoadTransposeMatrixfARB, glLoadTransposeMatrixfARB, NULL, _gloffset_LoadTransposeMatrixfARB),
    NAME_FUNC_OFFSET( 6215, glMultTransposeMatrixdARB, glMultTransposeMatrixdARB, NULL, _gloffset_MultTransposeMatrixdARB),
    NAME_FUNC_OFFSET( 6241, glMultTransposeMatrixfARB, glMultTransposeMatrixfARB, NULL, _gloffset_MultTransposeMatrixfARB),
    NAME_FUNC_OFFSET( 6267, glSampleCoverageARB, glSampleCoverageARB, NULL, _gloffset_SampleCoverageARB),
    NAME_FUNC_OFFSET( 6287, glCompressedTexImage1DARB, glCompressedTexImage1DARB, NULL, _gloffset_CompressedTexImage1DARB),
    NAME_FUNC_OFFSET( 6313, glCompressedTexImage2DARB, glCompressedTexImage2DARB, NULL, _gloffset_CompressedTexImage2DARB),
    NAME_FUNC_OFFSET( 6339, glCompressedTexImage3DARB, glCompressedTexImage3DARB, NULL, _gloffset_CompressedTexImage3DARB),
    NAME_FUNC_OFFSET( 6365, glCompressedTexSubImage1DARB, glCompressedTexSubImage1DARB, NULL, _gloffset_CompressedTexSubImage1DARB),
    NAME_FUNC_OFFSET( 6394, glCompressedTexSubImage2DARB, glCompressedTexSubImage2DARB, NULL, _gloffset_CompressedTexSubImage2DARB),
    NAME_FUNC_OFFSET( 6423, glCompressedTexSubImage3DARB, glCompressedTexSubImage3DARB, NULL, _gloffset_CompressedTexSubImage3DARB),
    NAME_FUNC_OFFSET( 6452, glGetCompressedTexImageARB, glGetCompressedTexImageARB, NULL, _gloffset_GetCompressedTexImageARB),
    NAME_FUNC_OFFSET( 6479, glDisableVertexAttribArrayARB, glDisableVertexAttribArrayARB, NULL, _gloffset_DisableVertexAttribArrayARB),
    NAME_FUNC_OFFSET( 6509, glEnableVertexAttribArrayARB, glEnableVertexAttribArrayARB, NULL, _gloffset_EnableVertexAttribArrayARB),
    NAME_FUNC_OFFSET( 6538, glGetProgramEnvParameterdvARB, glGetProgramEnvParameterdvARB, NULL, _gloffset_GetProgramEnvParameterdvARB),
    NAME_FUNC_OFFSET( 6568, glGetProgramEnvParameterfvARB, glGetProgramEnvParameterfvARB, NULL, _gloffset_GetProgramEnvParameterfvARB),
    NAME_FUNC_OFFSET( 6598, glGetProgramLocalParameterdvARB, glGetProgramLocalParameterdvARB, NULL, _gloffset_GetProgramLocalParameterdvARB),
    NAME_FUNC_OFFSET( 6630, glGetProgramLocalParameterfvARB, glGetProgramLocalParameterfvARB, NULL, _gloffset_GetProgramLocalParameterfvARB),
    NAME_FUNC_OFFSET( 6662, glGetProgramStringARB, glGetProgramStringARB, NULL, _gloffset_GetProgramStringARB),
    NAME_FUNC_OFFSET( 6684, glGetProgramivARB, glGetProgramivARB, NULL, _gloffset_GetProgramivARB),
    NAME_FUNC_OFFSET( 6702, glGetVertexAttribdvARB, glGetVertexAttribdvARB, NULL, _gloffset_GetVertexAttribdvARB),
    NAME_FUNC_OFFSET( 6725, glGetVertexAttribfvARB, glGetVertexAttribfvARB, NULL, _gloffset_GetVertexAttribfvARB),
    NAME_FUNC_OFFSET( 6748, glGetVertexAttribivARB, glGetVertexAttribivARB, NULL, _gloffset_GetVertexAttribivARB),
    NAME_FUNC_OFFSET( 6771, glProgramEnvParameter4dARB, glProgramEnvParameter4dARB, NULL, _gloffset_ProgramEnvParameter4dARB),
    NAME_FUNC_OFFSET( 6798, glProgramEnvParameter4dvARB, glProgramEnvParameter4dvARB, NULL, _gloffset_ProgramEnvParameter4dvARB),
    NAME_FUNC_OFFSET( 6826, glProgramEnvParameter4fARB, glProgramEnvParameter4fARB, NULL, _gloffset_ProgramEnvParameter4fARB),
    NAME_FUNC_OFFSET( 6853, glProgramEnvParameter4fvARB, glProgramEnvParameter4fvARB, NULL, _gloffset_ProgramEnvParameter4fvARB),
    NAME_FUNC_OFFSET( 6881, glProgramLocalParameter4dARB, glProgramLocalParameter4dARB, NULL, _gloffset_ProgramLocalParameter4dARB),
    NAME_FUNC_OFFSET( 6910, glProgramLocalParameter4dvARB, glProgramLocalParameter4dvARB, NULL, _gloffset_ProgramLocalParameter4dvARB),
    NAME_FUNC_OFFSET( 6940, glProgramLocalParameter4fARB, glProgramLocalParameter4fARB, NULL, _gloffset_ProgramLocalParameter4fARB),
    NAME_FUNC_OFFSET( 6969, glProgramLocalParameter4fvARB, glProgramLocalParameter4fvARB, NULL, _gloffset_ProgramLocalParameter4fvARB),
    NAME_FUNC_OFFSET( 6999, glProgramStringARB, glProgramStringARB, NULL, _gloffset_ProgramStringARB),
    NAME_FUNC_OFFSET( 7018, glVertexAttrib1dARB, glVertexAttrib1dARB, NULL, _gloffset_VertexAttrib1dARB),
    NAME_FUNC_OFFSET( 7038, glVertexAttrib1dvARB, glVertexAttrib1dvARB, NULL, _gloffset_VertexAttrib1dvARB),
    NAME_FUNC_OFFSET( 7059, glVertexAttrib1fARB, glVertexAttrib1fARB, NULL, _gloffset_VertexAttrib1fARB),
    NAME_FUNC_OFFSET( 7079, glVertexAttrib1fvARB, glVertexAttrib1fvARB, NULL, _gloffset_VertexAttrib1fvARB),
    NAME_FUNC_OFFSET( 7100, glVertexAttrib1sARB, glVertexAttrib1sARB, NULL, _gloffset_VertexAttrib1sARB),
    NAME_FUNC_OFFSET( 7120, glVertexAttrib1svARB, glVertexAttrib1svARB, NULL, _gloffset_VertexAttrib1svARB),
    NAME_FUNC_OFFSET( 7141, glVertexAttrib2dARB, glVertexAttrib2dARB, NULL, _gloffset_VertexAttrib2dARB),
    NAME_FUNC_OFFSET( 7161, glVertexAttrib2dvARB, glVertexAttrib2dvARB, NULL, _gloffset_VertexAttrib2dvARB),
    NAME_FUNC_OFFSET( 7182, glVertexAttrib2fARB, glVertexAttrib2fARB, NULL, _gloffset_VertexAttrib2fARB),
    NAME_FUNC_OFFSET( 7202, glVertexAttrib2fvARB, glVertexAttrib2fvARB, NULL, _gloffset_VertexAttrib2fvARB),
    NAME_FUNC_OFFSET( 7223, glVertexAttrib2sARB, glVertexAttrib2sARB, NULL, _gloffset_VertexAttrib2sARB),
    NAME_FUNC_OFFSET( 7243, glVertexAttrib2svARB, glVertexAttrib2svARB, NULL, _gloffset_VertexAttrib2svARB),
    NAME_FUNC_OFFSET( 7264, glVertexAttrib3dARB, glVertexAttrib3dARB, NULL, _gloffset_VertexAttrib3dARB),
    NAME_FUNC_OFFSET( 7284, glVertexAttrib3dvARB, glVertexAttrib3dvARB, NULL, _gloffset_VertexAttrib3dvARB),
    NAME_FUNC_OFFSET( 7305, glVertexAttrib3fARB, glVertexAttrib3fARB, NULL, _gloffset_VertexAttrib3fARB),
    NAME_FUNC_OFFSET( 7325, glVertexAttrib3fvARB, glVertexAttrib3fvARB, NULL, _gloffset_VertexAttrib3fvARB),
    NAME_FUNC_OFFSET( 7346, glVertexAttrib3sARB, glVertexAttrib3sARB, NULL, _gloffset_VertexAttrib3sARB),
    NAME_FUNC_OFFSET( 7366, glVertexAttrib3svARB, glVertexAttrib3svARB, NULL, _gloffset_VertexAttrib3svARB),
    NAME_FUNC_OFFSET( 7387, glVertexAttrib4NbvARB, glVertexAttrib4NbvARB, NULL, _gloffset_VertexAttrib4NbvARB),
    NAME_FUNC_OFFSET( 7409, glVertexAttrib4NivARB, glVertexAttrib4NivARB, NULL, _gloffset_VertexAttrib4NivARB),
    NAME_FUNC_OFFSET( 7431, glVertexAttrib4NsvARB, glVertexAttrib4NsvARB, NULL, _gloffset_VertexAttrib4NsvARB),
    NAME_FUNC_OFFSET( 7453, glVertexAttrib4NubARB, glVertexAttrib4NubARB, NULL, _gloffset_VertexAttrib4NubARB),
    NAME_FUNC_OFFSET( 7475, glVertexAttrib4NubvARB, glVertexAttrib4NubvARB, NULL, _gloffset_VertexAttrib4NubvARB),
    NAME_FUNC_OFFSET( 7498, glVertexAttrib4NuivARB, glVertexAttrib4NuivARB, NULL, _gloffset_VertexAttrib4NuivARB),
    NAME_FUNC_OFFSET( 7521, glVertexAttrib4NusvARB, glVertexAttrib4NusvARB, NULL, _gloffset_VertexAttrib4NusvARB),
    NAME_FUNC_OFFSET( 7544, glVertexAttrib4bvARB, glVertexAttrib4bvARB, NULL, _gloffset_VertexAttrib4bvARB),
    NAME_FUNC_OFFSET( 7565, glVertexAttrib4dARB, glVertexAttrib4dARB, NULL, _gloffset_VertexAttrib4dARB),
    NAME_FUNC_OFFSET( 7585, glVertexAttrib4dvARB, glVertexAttrib4dvARB, NULL, _gloffset_VertexAttrib4dvARB),
    NAME_FUNC_OFFSET( 7606, glVertexAttrib4fARB, glVertexAttrib4fARB, NULL, _gloffset_VertexAttrib4fARB),
    NAME_FUNC_OFFSET( 7626, glVertexAttrib4fvARB, glVertexAttrib4fvARB, NULL, _gloffset_VertexAttrib4fvARB),
    NAME_FUNC_OFFSET( 7647, glVertexAttrib4ivARB, glVertexAttrib4ivARB, NULL, _gloffset_VertexAttrib4ivARB),
    NAME_FUNC_OFFSET( 7668, glVertexAttrib4sARB, glVertexAttrib4sARB, NULL, _gloffset_VertexAttrib4sARB),
    NAME_FUNC_OFFSET( 7688, glVertexAttrib4svARB, glVertexAttrib4svARB, NULL, _gloffset_VertexAttrib4svARB),
    NAME_FUNC_OFFSET( 7709, glVertexAttrib4ubvARB, glVertexAttrib4ubvARB, NULL, _gloffset_VertexAttrib4ubvARB),
    NAME_FUNC_OFFSET( 7731, glVertexAttrib4uivARB, glVertexAttrib4uivARB, NULL, _gloffset_VertexAttrib4uivARB),
    NAME_FUNC_OFFSET( 7753, glVertexAttrib4usvARB, glVertexAttrib4usvARB, NULL, _gloffset_VertexAttrib4usvARB),
    NAME_FUNC_OFFSET( 7775, glVertexAttribPointerARB, glVertexAttribPointerARB, NULL, _gloffset_VertexAttribPointerARB),
    NAME_FUNC_OFFSET( 7800, glBindBufferARB, glBindBufferARB, NULL, _gloffset_BindBufferARB),
    NAME_FUNC_OFFSET( 7816, glBufferDataARB, glBufferDataARB, NULL, _gloffset_BufferDataARB),
    NAME_FUNC_OFFSET( 7832, glBufferSubDataARB, glBufferSubDataARB, NULL, _gloffset_BufferSubDataARB),
    NAME_FUNC_OFFSET( 7851, glDeleteBuffersARB, glDeleteBuffersARB, NULL, _gloffset_DeleteBuffersARB),
    NAME_FUNC_OFFSET( 7870, glGenBuffersARB, glGenBuffersARB, NULL, _gloffset_GenBuffersARB),
    NAME_FUNC_OFFSET( 7886, glGetBufferParameterivARB, glGetBufferParameterivARB, NULL, _gloffset_GetBufferParameterivARB),
    NAME_FUNC_OFFSET( 7912, glGetBufferPointervARB, glGetBufferPointervARB, NULL, _gloffset_GetBufferPointervARB),
    NAME_FUNC_OFFSET( 7935, glGetBufferSubDataARB, glGetBufferSubDataARB, NULL, _gloffset_GetBufferSubDataARB),
    NAME_FUNC_OFFSET( 7957, glIsBufferARB, glIsBufferARB, NULL, _gloffset_IsBufferARB),
    NAME_FUNC_OFFSET( 7971, glMapBufferARB, glMapBufferARB, NULL, _gloffset_MapBufferARB),
    NAME_FUNC_OFFSET( 7986, glUnmapBufferARB, glUnmapBufferARB, NULL, _gloffset_UnmapBufferARB),
    NAME_FUNC_OFFSET( 8003, glBeginQueryARB, glBeginQueryARB, NULL, _gloffset_BeginQueryARB),
    NAME_FUNC_OFFSET( 8019, glDeleteQueriesARB, glDeleteQueriesARB, NULL, _gloffset_DeleteQueriesARB),
    NAME_FUNC_OFFSET( 8038, glEndQueryARB, glEndQueryARB, NULL, _gloffset_EndQueryARB),
    NAME_FUNC_OFFSET( 8052, glGenQueriesARB, glGenQueriesARB, NULL, _gloffset_GenQueriesARB),
    NAME_FUNC_OFFSET( 8068, glGetQueryObjectivARB, glGetQueryObjectivARB, NULL, _gloffset_GetQueryObjectivARB),
    NAME_FUNC_OFFSET( 8090, glGetQueryObjectuivARB, glGetQueryObjectuivARB, NULL, _gloffset_GetQueryObjectuivARB),
    NAME_FUNC_OFFSET( 8113, glGetQueryivARB, glGetQueryivARB, NULL, _gloffset_GetQueryivARB),
    NAME_FUNC_OFFSET( 8129, glIsQueryARB, glIsQueryARB, NULL, _gloffset_IsQueryARB),
    NAME_FUNC_OFFSET( 8142, glAttachObjectARB, glAttachObjectARB, NULL, _gloffset_AttachObjectARB),
    NAME_FUNC_OFFSET( 8160, glCompileShaderARB, glCompileShaderARB, NULL, _gloffset_CompileShaderARB),
    NAME_FUNC_OFFSET( 8179, glCreateProgramObjectARB, glCreateProgramObjectARB, NULL, _gloffset_CreateProgramObjectARB),
    NAME_FUNC_OFFSET( 8204, glCreateShaderObjectARB, glCreateShaderObjectARB, NULL, _gloffset_CreateShaderObjectARB),
    NAME_FUNC_OFFSET( 8228, glDeleteObjectARB, glDeleteObjectARB, NULL, _gloffset_DeleteObjectARB),
    NAME_FUNC_OFFSET( 8246, glDetachObjectARB, glDetachObjectARB, NULL, _gloffset_DetachObjectARB),
    NAME_FUNC_OFFSET( 8264, glGetActiveUniformARB, glGetActiveUniformARB, NULL, _gloffset_GetActiveUniformARB),
    NAME_FUNC_OFFSET( 8286, glGetAttachedObjectsARB, glGetAttachedObjectsARB, NULL, _gloffset_GetAttachedObjectsARB),
    NAME_FUNC_OFFSET( 8310, glGetHandleARB, glGetHandleARB, NULL, _gloffset_GetHandleARB),
    NAME_FUNC_OFFSET( 8325, glGetInfoLogARB, glGetInfoLogARB, NULL, _gloffset_GetInfoLogARB),
    NAME_FUNC_OFFSET( 8341, glGetObjectParameterfvARB, glGetObjectParameterfvARB, NULL, _gloffset_GetObjectParameterfvARB),
    NAME_FUNC_OFFSET( 8367, glGetObjectParameterivARB, glGetObjectParameterivARB, NULL, _gloffset_GetObjectParameterivARB),
    NAME_FUNC_OFFSET( 8393, glGetShaderSourceARB, glGetShaderSourceARB, NULL, _gloffset_GetShaderSourceARB),
    NAME_FUNC_OFFSET( 8414, glGetUniformLocationARB, glGetUniformLocationARB, NULL, _gloffset_GetUniformLocationARB),
    NAME_FUNC_OFFSET( 8438, glGetUniformfvARB, glGetUniformfvARB, NULL, _gloffset_GetUniformfvARB),
    NAME_FUNC_OFFSET( 8456, glGetUniformivARB, glGetUniformivARB, NULL, _gloffset_GetUniformivARB),
    NAME_FUNC_OFFSET( 8474, glLinkProgramARB, glLinkProgramARB, NULL, _gloffset_LinkProgramARB),
    NAME_FUNC_OFFSET( 8491, glShaderSourceARB, glShaderSourceARB, NULL, _gloffset_ShaderSourceARB),
    NAME_FUNC_OFFSET( 8509, glUniform1fARB, glUniform1fARB, NULL, _gloffset_Uniform1fARB),
    NAME_FUNC_OFFSET( 8524, glUniform1fvARB, glUniform1fvARB, NULL, _gloffset_Uniform1fvARB),
    NAME_FUNC_OFFSET( 8540, glUniform1iARB, glUniform1iARB, NULL, _gloffset_Uniform1iARB),
    NAME_FUNC_OFFSET( 8555, glUniform1ivARB, glUniform1ivARB, NULL, _gloffset_Uniform1ivARB),
    NAME_FUNC_OFFSET( 8571, glUniform2fARB, glUniform2fARB, NULL, _gloffset_Uniform2fARB),
    NAME_FUNC_OFFSET( 8586, glUniform2fvARB, glUniform2fvARB, NULL, _gloffset_Uniform2fvARB),
    NAME_FUNC_OFFSET( 8602, glUniform2iARB, glUniform2iARB, NULL, _gloffset_Uniform2iARB),
    NAME_FUNC_OFFSET( 8617, glUniform2ivARB, glUniform2ivARB, NULL, _gloffset_Uniform2ivARB),
    NAME_FUNC_OFFSET( 8633, glUniform3fARB, glUniform3fARB, NULL, _gloffset_Uniform3fARB),
    NAME_FUNC_OFFSET( 8648, glUniform3fvARB, glUniform3fvARB, NULL, _gloffset_Uniform3fvARB),
    NAME_FUNC_OFFSET( 8664, glUniform3iARB, glUniform3iARB, NULL, _gloffset_Uniform3iARB),
    NAME_FUNC_OFFSET( 8679, glUniform3ivARB, glUniform3ivARB, NULL, _gloffset_Uniform3ivARB),
    NAME_FUNC_OFFSET( 8695, glUniform4fARB, glUniform4fARB, NULL, _gloffset_Uniform4fARB),
    NAME_FUNC_OFFSET( 8710, glUniform4fvARB, glUniform4fvARB, NULL, _gloffset_Uniform4fvARB),
    NAME_FUNC_OFFSET( 8726, glUniform4iARB, glUniform4iARB, NULL, _gloffset_Uniform4iARB),
    NAME_FUNC_OFFSET( 8741, glUniform4ivARB, glUniform4ivARB, NULL, _gloffset_Uniform4ivARB),
    NAME_FUNC_OFFSET( 8757, glUniformMatrix2fvARB, glUniformMatrix2fvARB, NULL, _gloffset_UniformMatrix2fvARB),
    NAME_FUNC_OFFSET( 8779, glUniformMatrix3fvARB, glUniformMatrix3fvARB, NULL, _gloffset_UniformMatrix3fvARB),
    NAME_FUNC_OFFSET( 8801, glUniformMatrix4fvARB, glUniformMatrix4fvARB, NULL, _gloffset_UniformMatrix4fvARB),
    NAME_FUNC_OFFSET( 8823, glUseProgramObjectARB, glUseProgramObjectARB, NULL, _gloffset_UseProgramObjectARB),
    NAME_FUNC_OFFSET( 8845, glValidateProgramARB, glValidateProgramARB, NULL, _gloffset_ValidateProgramARB),
    NAME_FUNC_OFFSET( 8866, glBindAttribLocationARB, glBindAttribLocationARB, NULL, _gloffset_BindAttribLocationARB),
    NAME_FUNC_OFFSET( 8890, glGetActiveAttribARB, glGetActiveAttribARB, NULL, _gloffset_GetActiveAttribARB),
    NAME_FUNC_OFFSET( 8911, glGetAttribLocationARB, glGetAttribLocationARB, NULL, _gloffset_GetAttribLocationARB),
    NAME_FUNC_OFFSET( 8934, glDrawBuffersARB, glDrawBuffersARB, NULL, _gloffset_DrawBuffersARB),
    NAME_FUNC_OFFSET( 8951, glPolygonOffsetEXT, glPolygonOffsetEXT, NULL, _gloffset_PolygonOffsetEXT),
    NAME_FUNC_OFFSET( 8970, gl_dispatch_stub_562, gl_dispatch_stub_562, NULL, _gloffset_GetPixelTexGenParameterfvSGIS),
    NAME_FUNC_OFFSET( 9002, gl_dispatch_stub_563, gl_dispatch_stub_563, NULL, _gloffset_GetPixelTexGenParameterivSGIS),
    NAME_FUNC_OFFSET( 9034, gl_dispatch_stub_564, gl_dispatch_stub_564, NULL, _gloffset_PixelTexGenParameterfSGIS),
    NAME_FUNC_OFFSET( 9062, gl_dispatch_stub_565, gl_dispatch_stub_565, NULL, _gloffset_PixelTexGenParameterfvSGIS),
    NAME_FUNC_OFFSET( 9091, gl_dispatch_stub_566, gl_dispatch_stub_566, NULL, _gloffset_PixelTexGenParameteriSGIS),
    NAME_FUNC_OFFSET( 9119, gl_dispatch_stub_567, gl_dispatch_stub_567, NULL, _gloffset_PixelTexGenParameterivSGIS),
    NAME_FUNC_OFFSET( 9148, gl_dispatch_stub_568, gl_dispatch_stub_568, NULL, _gloffset_SampleMaskSGIS),
    NAME_FUNC_OFFSET( 9165, gl_dispatch_stub_569, gl_dispatch_stub_569, NULL, _gloffset_SamplePatternSGIS),
    NAME_FUNC_OFFSET( 9185, glColorPointerEXT, glColorPointerEXT, NULL, _gloffset_ColorPointerEXT),
    NAME_FUNC_OFFSET( 9203, glEdgeFlagPointerEXT, glEdgeFlagPointerEXT, NULL, _gloffset_EdgeFlagPointerEXT),
    NAME_FUNC_OFFSET( 9224, glIndexPointerEXT, glIndexPointerEXT, NULL, _gloffset_IndexPointerEXT),
    NAME_FUNC_OFFSET( 9242, glNormalPointerEXT, glNormalPointerEXT, NULL, _gloffset_NormalPointerEXT),
    NAME_FUNC_OFFSET( 9261, glTexCoordPointerEXT, glTexCoordPointerEXT, NULL, _gloffset_TexCoordPointerEXT),
    NAME_FUNC_OFFSET( 9282, glVertexPointerEXT, glVertexPointerEXT, NULL, _gloffset_VertexPointerEXT),
    NAME_FUNC_OFFSET( 9301, glPointParameterfEXT, glPointParameterfEXT, NULL, _gloffset_PointParameterfEXT),
    NAME_FUNC_OFFSET( 9322, glPointParameterfvEXT, glPointParameterfvEXT, NULL, _gloffset_PointParameterfvEXT),
    NAME_FUNC_OFFSET( 9344, glLockArraysEXT, glLockArraysEXT, NULL, _gloffset_LockArraysEXT),
    NAME_FUNC_OFFSET( 9360, glUnlockArraysEXT, glUnlockArraysEXT, NULL, _gloffset_UnlockArraysEXT),
    NAME_FUNC_OFFSET( 9378, gl_dispatch_stub_580, gl_dispatch_stub_580, NULL, _gloffset_CullParameterdvEXT),
    NAME_FUNC_OFFSET( 9399, gl_dispatch_stub_581, gl_dispatch_stub_581, NULL, _gloffset_CullParameterfvEXT),
    NAME_FUNC_OFFSET( 9420, glSecondaryColor3bEXT, glSecondaryColor3bEXT, NULL, _gloffset_SecondaryColor3bEXT),
    NAME_FUNC_OFFSET( 9442, glSecondaryColor3bvEXT, glSecondaryColor3bvEXT, NULL, _gloffset_SecondaryColor3bvEXT),
    NAME_FUNC_OFFSET( 9465, glSecondaryColor3dEXT, glSecondaryColor3dEXT, NULL, _gloffset_SecondaryColor3dEXT),
    NAME_FUNC_OFFSET( 9487, glSecondaryColor3dvEXT, glSecondaryColor3dvEXT, NULL, _gloffset_SecondaryColor3dvEXT),
    NAME_FUNC_OFFSET( 9510, glSecondaryColor3fEXT, glSecondaryColor3fEXT, NULL, _gloffset_SecondaryColor3fEXT),
    NAME_FUNC_OFFSET( 9532, glSecondaryColor3fvEXT, glSecondaryColor3fvEXT, NULL, _gloffset_SecondaryColor3fvEXT),
    NAME_FUNC_OFFSET( 9555, glSecondaryColor3iEXT, glSecondaryColor3iEXT, NULL, _gloffset_SecondaryColor3iEXT),
    NAME_FUNC_OFFSET( 9577, glSecondaryColor3ivEXT, glSecondaryColor3ivEXT, NULL, _gloffset_SecondaryColor3ivEXT),
    NAME_FUNC_OFFSET( 9600, glSecondaryColor3sEXT, glSecondaryColor3sEXT, NULL, _gloffset_SecondaryColor3sEXT),
    NAME_FUNC_OFFSET( 9622, glSecondaryColor3svEXT, glSecondaryColor3svEXT, NULL, _gloffset_SecondaryColor3svEXT),
    NAME_FUNC_OFFSET( 9645, glSecondaryColor3ubEXT, glSecondaryColor3ubEXT, NULL, _gloffset_SecondaryColor3ubEXT),
    NAME_FUNC_OFFSET( 9668, glSecondaryColor3ubvEXT, glSecondaryColor3ubvEXT, NULL, _gloffset_SecondaryColor3ubvEXT),
    NAME_FUNC_OFFSET( 9692, glSecondaryColor3uiEXT, glSecondaryColor3uiEXT, NULL, _gloffset_SecondaryColor3uiEXT),
    NAME_FUNC_OFFSET( 9715, glSecondaryColor3uivEXT, glSecondaryColor3uivEXT, NULL, _gloffset_SecondaryColor3uivEXT),
    NAME_FUNC_OFFSET( 9739, glSecondaryColor3usEXT, glSecondaryColor3usEXT, NULL, _gloffset_SecondaryColor3usEXT),
    NAME_FUNC_OFFSET( 9762, glSecondaryColor3usvEXT, glSecondaryColor3usvEXT, NULL, _gloffset_SecondaryColor3usvEXT),
    NAME_FUNC_OFFSET( 9786, glSecondaryColorPointerEXT, glSecondaryColorPointerEXT, NULL, _gloffset_SecondaryColorPointerEXT),
    NAME_FUNC_OFFSET( 9813, glMultiDrawArraysEXT, glMultiDrawArraysEXT, NULL, _gloffset_MultiDrawArraysEXT),
    NAME_FUNC_OFFSET( 9834, glMultiDrawElementsEXT, glMultiDrawElementsEXT, NULL, _gloffset_MultiDrawElementsEXT),
    NAME_FUNC_OFFSET( 9857, glFogCoordPointerEXT, glFogCoordPointerEXT, NULL, _gloffset_FogCoordPointerEXT),
    NAME_FUNC_OFFSET( 9878, glFogCoorddEXT, glFogCoorddEXT, NULL, _gloffset_FogCoorddEXT),
    NAME_FUNC_OFFSET( 9893, glFogCoorddvEXT, glFogCoorddvEXT, NULL, _gloffset_FogCoorddvEXT),
    NAME_FUNC_OFFSET( 9909, glFogCoordfEXT, glFogCoordfEXT, NULL, _gloffset_FogCoordfEXT),
    NAME_FUNC_OFFSET( 9924, glFogCoordfvEXT, glFogCoordfvEXT, NULL, _gloffset_FogCoordfvEXT),
    NAME_FUNC_OFFSET( 9940, gl_dispatch_stub_606, gl_dispatch_stub_606, NULL, _gloffset_PixelTexGenSGIX),
    NAME_FUNC_OFFSET( 9958, glBlendFuncSeparateEXT, glBlendFuncSeparateEXT, NULL, _gloffset_BlendFuncSeparateEXT),
    NAME_FUNC_OFFSET( 9981, glFlushVertexArrayRangeNV, glFlushVertexArrayRangeNV, NULL, _gloffset_FlushVertexArrayRangeNV),
    NAME_FUNC_OFFSET(10007, glVertexArrayRangeNV, glVertexArrayRangeNV, NULL, _gloffset_VertexArrayRangeNV),
    NAME_FUNC_OFFSET(10028, glCombinerInputNV, glCombinerInputNV, NULL, _gloffset_CombinerInputNV),
    NAME_FUNC_OFFSET(10046, glCombinerOutputNV, glCombinerOutputNV, NULL, _gloffset_CombinerOutputNV),
    NAME_FUNC_OFFSET(10065, glCombinerParameterfNV, glCombinerParameterfNV, NULL, _gloffset_CombinerParameterfNV),
    NAME_FUNC_OFFSET(10088, glCombinerParameterfvNV, glCombinerParameterfvNV, NULL, _gloffset_CombinerParameterfvNV),
    NAME_FUNC_OFFSET(10112, glCombinerParameteriNV, glCombinerParameteriNV, NULL, _gloffset_CombinerParameteriNV),
    NAME_FUNC_OFFSET(10135, glCombinerParameterivNV, glCombinerParameterivNV, NULL, _gloffset_CombinerParameterivNV),
    NAME_FUNC_OFFSET(10159, glFinalCombinerInputNV, glFinalCombinerInputNV, NULL, _gloffset_FinalCombinerInputNV),
    NAME_FUNC_OFFSET(10182, glGetCombinerInputParameterfvNV, glGetCombinerInputParameterfvNV, NULL, _gloffset_GetCombinerInputParameterfvNV),
    NAME_FUNC_OFFSET(10214, glGetCombinerInputParameterivNV, glGetCombinerInputParameterivNV, NULL, _gloffset_GetCombinerInputParameterivNV),
    NAME_FUNC_OFFSET(10246, glGetCombinerOutputParameterfvNV, glGetCombinerOutputParameterfvNV, NULL, _gloffset_GetCombinerOutputParameterfvNV),
    NAME_FUNC_OFFSET(10279, glGetCombinerOutputParameterivNV, glGetCombinerOutputParameterivNV, NULL, _gloffset_GetCombinerOutputParameterivNV),
    NAME_FUNC_OFFSET(10312, glGetFinalCombinerInputParameterfvNV, glGetFinalCombinerInputParameterfvNV, NULL, _gloffset_GetFinalCombinerInputParameterfvNV),
    NAME_FUNC_OFFSET(10349, glGetFinalCombinerInputParameterivNV, glGetFinalCombinerInputParameterivNV, NULL, _gloffset_GetFinalCombinerInputParameterivNV),
    NAME_FUNC_OFFSET(10386, glResizeBuffersMESA, glResizeBuffersMESA, NULL, _gloffset_ResizeBuffersMESA),
    NAME_FUNC_OFFSET(10406, glWindowPos2dMESA, glWindowPos2dMESA, NULL, _gloffset_WindowPos2dMESA),
    NAME_FUNC_OFFSET(10424, glWindowPos2dvMESA, glWindowPos2dvMESA, NULL, _gloffset_WindowPos2dvMESA),
    NAME_FUNC_OFFSET(10443, glWindowPos2fMESA, glWindowPos2fMESA, NULL, _gloffset_WindowPos2fMESA),
    NAME_FUNC_OFFSET(10461, glWindowPos2fvMESA, glWindowPos2fvMESA, NULL, _gloffset_WindowPos2fvMESA),
    NAME_FUNC_OFFSET(10480, glWindowPos2iMESA, glWindowPos2iMESA, NULL, _gloffset_WindowPos2iMESA),
    NAME_FUNC_OFFSET(10498, glWindowPos2ivMESA, glWindowPos2ivMESA, NULL, _gloffset_WindowPos2ivMESA),
    NAME_FUNC_OFFSET(10517, glWindowPos2sMESA, glWindowPos2sMESA, NULL, _gloffset_WindowPos2sMESA),
    NAME_FUNC_OFFSET(10535, glWindowPos2svMESA, glWindowPos2svMESA, NULL, _gloffset_WindowPos2svMESA),
    NAME_FUNC_OFFSET(10554, glWindowPos3dMESA, glWindowPos3dMESA, NULL, _gloffset_WindowPos3dMESA),
    NAME_FUNC_OFFSET(10572, glWindowPos3dvMESA, glWindowPos3dvMESA, NULL, _gloffset_WindowPos3dvMESA),
    NAME_FUNC_OFFSET(10591, glWindowPos3fMESA, glWindowPos3fMESA, NULL, _gloffset_WindowPos3fMESA),
    NAME_FUNC_OFFSET(10609, glWindowPos3fvMESA, glWindowPos3fvMESA, NULL, _gloffset_WindowPos3fvMESA),
    NAME_FUNC_OFFSET(10628, glWindowPos3iMESA, glWindowPos3iMESA, NULL, _gloffset_WindowPos3iMESA),
    NAME_FUNC_OFFSET(10646, glWindowPos3ivMESA, glWindowPos3ivMESA, NULL, _gloffset_WindowPos3ivMESA),
    NAME_FUNC_OFFSET(10665, glWindowPos3sMESA, glWindowPos3sMESA, NULL, _gloffset_WindowPos3sMESA),
    NAME_FUNC_OFFSET(10683, glWindowPos3svMESA, glWindowPos3svMESA, NULL, _gloffset_WindowPos3svMESA),
    NAME_FUNC_OFFSET(10702, glWindowPos4dMESA, glWindowPos4dMESA, NULL, _gloffset_WindowPos4dMESA),
    NAME_FUNC_OFFSET(10720, glWindowPos4dvMESA, glWindowPos4dvMESA, NULL, _gloffset_WindowPos4dvMESA),
    NAME_FUNC_OFFSET(10739, glWindowPos4fMESA, glWindowPos4fMESA, NULL, _gloffset_WindowPos4fMESA),
    NAME_FUNC_OFFSET(10757, glWindowPos4fvMESA, glWindowPos4fvMESA, NULL, _gloffset_WindowPos4fvMESA),
    NAME_FUNC_OFFSET(10776, glWindowPos4iMESA, glWindowPos4iMESA, NULL, _gloffset_WindowPos4iMESA),
    NAME_FUNC_OFFSET(10794, glWindowPos4ivMESA, glWindowPos4ivMESA, NULL, _gloffset_WindowPos4ivMESA),
    NAME_FUNC_OFFSET(10813, glWindowPos4sMESA, glWindowPos4sMESA, NULL, _gloffset_WindowPos4sMESA),
    NAME_FUNC_OFFSET(10831, glWindowPos4svMESA, glWindowPos4svMESA, NULL, _gloffset_WindowPos4svMESA),
    NAME_FUNC_OFFSET(10850, gl_dispatch_stub_648, gl_dispatch_stub_648, NULL, _gloffset_MultiModeDrawArraysIBM),
    NAME_FUNC_OFFSET(10875, gl_dispatch_stub_649, gl_dispatch_stub_649, NULL, _gloffset_MultiModeDrawElementsIBM),
    NAME_FUNC_OFFSET(10902, gl_dispatch_stub_650, gl_dispatch_stub_650, NULL, _gloffset_DeleteFencesNV),
    NAME_FUNC_OFFSET(10919, gl_dispatch_stub_651, gl_dispatch_stub_651, NULL, _gloffset_FinishFenceNV),
    NAME_FUNC_OFFSET(10935, gl_dispatch_stub_652, gl_dispatch_stub_652, NULL, _gloffset_GenFencesNV),
    NAME_FUNC_OFFSET(10949, gl_dispatch_stub_653, gl_dispatch_stub_653, NULL, _gloffset_GetFenceivNV),
    NAME_FUNC_OFFSET(10964, gl_dispatch_stub_654, gl_dispatch_stub_654, NULL, _gloffset_IsFenceNV),
    NAME_FUNC_OFFSET(10976, gl_dispatch_stub_655, gl_dispatch_stub_655, NULL, _gloffset_SetFenceNV),
    NAME_FUNC_OFFSET(10989, gl_dispatch_stub_656, gl_dispatch_stub_656, NULL, _gloffset_TestFenceNV),
    NAME_FUNC_OFFSET(11003, glAreProgramsResidentNV, glAreProgramsResidentNV, NULL, _gloffset_AreProgramsResidentNV),
    NAME_FUNC_OFFSET(11027, glBindProgramNV, glBindProgramNV, NULL, _gloffset_BindProgramNV),
    NAME_FUNC_OFFSET(11043, glDeleteProgramsNV, glDeleteProgramsNV, NULL, _gloffset_DeleteProgramsNV),
    NAME_FUNC_OFFSET(11062, glExecuteProgramNV, glExecuteProgramNV, NULL, _gloffset_ExecuteProgramNV),
    NAME_FUNC_OFFSET(11081, glGenProgramsNV, glGenProgramsNV, NULL, _gloffset_GenProgramsNV),
    NAME_FUNC_OFFSET(11097, glGetProgramParameterdvNV, glGetProgramParameterdvNV, NULL, _gloffset_GetProgramParameterdvNV),
    NAME_FUNC_OFFSET(11123, glGetProgramParameterfvNV, glGetProgramParameterfvNV, NULL, _gloffset_GetProgramParameterfvNV),
    NAME_FUNC_OFFSET(11149, glGetProgramStringNV, glGetProgramStringNV, NULL, _gloffset_GetProgramStringNV),
    NAME_FUNC_OFFSET(11170, glGetProgramivNV, glGetProgramivNV, NULL, _gloffset_GetProgramivNV),
    NAME_FUNC_OFFSET(11187, glGetTrackMatrixivNV, glGetTrackMatrixivNV, NULL, _gloffset_GetTrackMatrixivNV),
    NAME_FUNC_OFFSET(11208, glGetVertexAttribPointervNV, glGetVertexAttribPointervNV, NULL, _gloffset_GetVertexAttribPointervNV),
    NAME_FUNC_OFFSET(11236, glGetVertexAttribdvNV, glGetVertexAttribdvNV, NULL, _gloffset_GetVertexAttribdvNV),
    NAME_FUNC_OFFSET(11258, glGetVertexAttribfvNV, glGetVertexAttribfvNV, NULL, _gloffset_GetVertexAttribfvNV),
    NAME_FUNC_OFFSET(11280, glGetVertexAttribivNV, glGetVertexAttribivNV, NULL, _gloffset_GetVertexAttribivNV),
    NAME_FUNC_OFFSET(11302, glIsProgramNV, glIsProgramNV, NULL, _gloffset_IsProgramNV),
    NAME_FUNC_OFFSET(11316, glLoadProgramNV, glLoadProgramNV, NULL, _gloffset_LoadProgramNV),
    NAME_FUNC_OFFSET(11332, glProgramParameters4dvNV, glProgramParameters4dvNV, NULL, _gloffset_ProgramParameters4dvNV),
    NAME_FUNC_OFFSET(11357, glProgramParameters4fvNV, glProgramParameters4fvNV, NULL, _gloffset_ProgramParameters4fvNV),
    NAME_FUNC_OFFSET(11382, glRequestResidentProgramsNV, glRequestResidentProgramsNV, NULL, _gloffset_RequestResidentProgramsNV),
    NAME_FUNC_OFFSET(11410, glTrackMatrixNV, glTrackMatrixNV, NULL, _gloffset_TrackMatrixNV),
    NAME_FUNC_OFFSET(11426, glVertexAttrib1dNV, glVertexAttrib1dNV, NULL, _gloffset_VertexAttrib1dNV),
    NAME_FUNC_OFFSET(11445, glVertexAttrib1dvNV, glVertexAttrib1dvNV, NULL, _gloffset_VertexAttrib1dvNV),
    NAME_FUNC_OFFSET(11465, glVertexAttrib1fNV, glVertexAttrib1fNV, NULL, _gloffset_VertexAttrib1fNV),
    NAME_FUNC_OFFSET(11484, glVertexAttrib1fvNV, glVertexAttrib1fvNV, NULL, _gloffset_VertexAttrib1fvNV),
    NAME_FUNC_OFFSET(11504, glVertexAttrib1sNV, glVertexAttrib1sNV, NULL, _gloffset_VertexAttrib1sNV),
    NAME_FUNC_OFFSET(11523, glVertexAttrib1svNV, glVertexAttrib1svNV, NULL, _gloffset_VertexAttrib1svNV),
    NAME_FUNC_OFFSET(11543, glVertexAttrib2dNV, glVertexAttrib2dNV, NULL, _gloffset_VertexAttrib2dNV),
    NAME_FUNC_OFFSET(11562, glVertexAttrib2dvNV, glVertexAttrib2dvNV, NULL, _gloffset_VertexAttrib2dvNV),
    NAME_FUNC_OFFSET(11582, glVertexAttrib2fNV, glVertexAttrib2fNV, NULL, _gloffset_VertexAttrib2fNV),
    NAME_FUNC_OFFSET(11601, glVertexAttrib2fvNV, glVertexAttrib2fvNV, NULL, _gloffset_VertexAttrib2fvNV),
    NAME_FUNC_OFFSET(11621, glVertexAttrib2sNV, glVertexAttrib2sNV, NULL, _gloffset_VertexAttrib2sNV),
    NAME_FUNC_OFFSET(11640, glVertexAttrib2svNV, glVertexAttrib2svNV, NULL, _gloffset_VertexAttrib2svNV),
    NAME_FUNC_OFFSET(11660, glVertexAttrib3dNV, glVertexAttrib3dNV, NULL, _gloffset_VertexAttrib3dNV),
    NAME_FUNC_OFFSET(11679, glVertexAttrib3dvNV, glVertexAttrib3dvNV, NULL, _gloffset_VertexAttrib3dvNV),
    NAME_FUNC_OFFSET(11699, glVertexAttrib3fNV, glVertexAttrib3fNV, NULL, _gloffset_VertexAttrib3fNV),
    NAME_FUNC_OFFSET(11718, glVertexAttrib3fvNV, glVertexAttrib3fvNV, NULL, _gloffset_VertexAttrib3fvNV),
    NAME_FUNC_OFFSET(11738, glVertexAttrib3sNV, glVertexAttrib3sNV, NULL, _gloffset_VertexAttrib3sNV),
    NAME_FUNC_OFFSET(11757, glVertexAttrib3svNV, glVertexAttrib3svNV, NULL, _gloffset_VertexAttrib3svNV),
    NAME_FUNC_OFFSET(11777, glVertexAttrib4dNV, glVertexAttrib4dNV, NULL, _gloffset_VertexAttrib4dNV),
    NAME_FUNC_OFFSET(11796, glVertexAttrib4dvNV, glVertexAttrib4dvNV, NULL, _gloffset_VertexAttrib4dvNV),
    NAME_FUNC_OFFSET(11816, glVertexAttrib4fNV, glVertexAttrib4fNV, NULL, _gloffset_VertexAttrib4fNV),
    NAME_FUNC_OFFSET(11835, glVertexAttrib4fvNV, glVertexAttrib4fvNV, NULL, _gloffset_VertexAttrib4fvNV),
    NAME_FUNC_OFFSET(11855, glVertexAttrib4sNV, glVertexAttrib4sNV, NULL, _gloffset_VertexAttrib4sNV),
    NAME_FUNC_OFFSET(11874, glVertexAttrib4svNV, glVertexAttrib4svNV, NULL, _gloffset_VertexAttrib4svNV),
    NAME_FUNC_OFFSET(11894, glVertexAttrib4ubNV, glVertexAttrib4ubNV, NULL, _gloffset_VertexAttrib4ubNV),
    NAME_FUNC_OFFSET(11914, glVertexAttrib4ubvNV, glVertexAttrib4ubvNV, NULL, _gloffset_VertexAttrib4ubvNV),
    NAME_FUNC_OFFSET(11935, glVertexAttribPointerNV, glVertexAttribPointerNV, NULL, _gloffset_VertexAttribPointerNV),
    NAME_FUNC_OFFSET(11959, glVertexAttribs1dvNV, glVertexAttribs1dvNV, NULL, _gloffset_VertexAttribs1dvNV),
    NAME_FUNC_OFFSET(11980, glVertexAttribs1fvNV, glVertexAttribs1fvNV, NULL, _gloffset_VertexAttribs1fvNV),
    NAME_FUNC_OFFSET(12001, glVertexAttribs1svNV, glVertexAttribs1svNV, NULL, _gloffset_VertexAttribs1svNV),
    NAME_FUNC_OFFSET(12022, glVertexAttribs2dvNV, glVertexAttribs2dvNV, NULL, _gloffset_VertexAttribs2dvNV),
    NAME_FUNC_OFFSET(12043, glVertexAttribs2fvNV, glVertexAttribs2fvNV, NULL, _gloffset_VertexAttribs2fvNV),
    NAME_FUNC_OFFSET(12064, glVertexAttribs2svNV, glVertexAttribs2svNV, NULL, _gloffset_VertexAttribs2svNV),
    NAME_FUNC_OFFSET(12085, glVertexAttribs3dvNV, glVertexAttribs3dvNV, NULL, _gloffset_VertexAttribs3dvNV),
    NAME_FUNC_OFFSET(12106, glVertexAttribs3fvNV, glVertexAttribs3fvNV, NULL, _gloffset_VertexAttribs3fvNV),
    NAME_FUNC_OFFSET(12127, glVertexAttribs3svNV, glVertexAttribs3svNV, NULL, _gloffset_VertexAttribs3svNV),
    NAME_FUNC_OFFSET(12148, glVertexAttribs4dvNV, glVertexAttribs4dvNV, NULL, _gloffset_VertexAttribs4dvNV),
    NAME_FUNC_OFFSET(12169, glVertexAttribs4fvNV, glVertexAttribs4fvNV, NULL, _gloffset_VertexAttribs4fvNV),
    NAME_FUNC_OFFSET(12190, glVertexAttribs4svNV, glVertexAttribs4svNV, NULL, _gloffset_VertexAttribs4svNV),
    NAME_FUNC_OFFSET(12211, glVertexAttribs4ubvNV, glVertexAttribs4ubvNV, NULL, _gloffset_VertexAttribs4ubvNV),
    NAME_FUNC_OFFSET(12233, glAlphaFragmentOp1ATI, glAlphaFragmentOp1ATI, NULL, _gloffset_AlphaFragmentOp1ATI),
    NAME_FUNC_OFFSET(12255, glAlphaFragmentOp2ATI, glAlphaFragmentOp2ATI, NULL, _gloffset_AlphaFragmentOp2ATI),
    NAME_FUNC_OFFSET(12277, glAlphaFragmentOp3ATI, glAlphaFragmentOp3ATI, NULL, _gloffset_AlphaFragmentOp3ATI),
    NAME_FUNC_OFFSET(12299, glBeginFragmentShaderATI, glBeginFragmentShaderATI, NULL, _gloffset_BeginFragmentShaderATI),
    NAME_FUNC_OFFSET(12324, glBindFragmentShaderATI, glBindFragmentShaderATI, NULL, _gloffset_BindFragmentShaderATI),
    NAME_FUNC_OFFSET(12348, glColorFragmentOp1ATI, glColorFragmentOp1ATI, NULL, _gloffset_ColorFragmentOp1ATI),
    NAME_FUNC_OFFSET(12370, glColorFragmentOp2ATI, glColorFragmentOp2ATI, NULL, _gloffset_ColorFragmentOp2ATI),
    NAME_FUNC_OFFSET(12392, glColorFragmentOp3ATI, glColorFragmentOp3ATI, NULL, _gloffset_ColorFragmentOp3ATI),
    NAME_FUNC_OFFSET(12414, glDeleteFragmentShaderATI, glDeleteFragmentShaderATI, NULL, _gloffset_DeleteFragmentShaderATI),
    NAME_FUNC_OFFSET(12440, glEndFragmentShaderATI, glEndFragmentShaderATI, NULL, _gloffset_EndFragmentShaderATI),
    NAME_FUNC_OFFSET(12463, glGenFragmentShadersATI, glGenFragmentShadersATI, NULL, _gloffset_GenFragmentShadersATI),
    NAME_FUNC_OFFSET(12487, glPassTexCoordATI, glPassTexCoordATI, NULL, _gloffset_PassTexCoordATI),
    NAME_FUNC_OFFSET(12505, glSampleMapATI, glSampleMapATI, NULL, _gloffset_SampleMapATI),
    NAME_FUNC_OFFSET(12520, glSetFragmentShaderConstantATI, glSetFragmentShaderConstantATI, NULL, _gloffset_SetFragmentShaderConstantATI),
    NAME_FUNC_OFFSET(12551, glPointParameteriNV, glPointParameteriNV, NULL, _gloffset_PointParameteriNV),
    NAME_FUNC_OFFSET(12571, glPointParameterivNV, glPointParameterivNV, NULL, _gloffset_PointParameterivNV),
    NAME_FUNC_OFFSET(12592, gl_dispatch_stub_733, gl_dispatch_stub_733, NULL, _gloffset_ActiveStencilFaceEXT),
    NAME_FUNC_OFFSET(12615, gl_dispatch_stub_734, gl_dispatch_stub_734, NULL, _gloffset_BindVertexArrayAPPLE),
    NAME_FUNC_OFFSET(12638, gl_dispatch_stub_735, gl_dispatch_stub_735, NULL, _gloffset_DeleteVertexArraysAPPLE),
    NAME_FUNC_OFFSET(12664, gl_dispatch_stub_736, gl_dispatch_stub_736, NULL, _gloffset_GenVertexArraysAPPLE),
    NAME_FUNC_OFFSET(12687, gl_dispatch_stub_737, gl_dispatch_stub_737, NULL, _gloffset_IsVertexArrayAPPLE),
    NAME_FUNC_OFFSET(12708, glGetProgramNamedParameterdvNV, glGetProgramNamedParameterdvNV, NULL, _gloffset_GetProgramNamedParameterdvNV),
    NAME_FUNC_OFFSET(12739, glGetProgramNamedParameterfvNV, glGetProgramNamedParameterfvNV, NULL, _gloffset_GetProgramNamedParameterfvNV),
    NAME_FUNC_OFFSET(12770, glProgramNamedParameter4dNV, glProgramNamedParameter4dNV, NULL, _gloffset_ProgramNamedParameter4dNV),
    NAME_FUNC_OFFSET(12798, glProgramNamedParameter4dvNV, glProgramNamedParameter4dvNV, NULL, _gloffset_ProgramNamedParameter4dvNV),
    NAME_FUNC_OFFSET(12827, glProgramNamedParameter4fNV, glProgramNamedParameter4fNV, NULL, _gloffset_ProgramNamedParameter4fNV),
    NAME_FUNC_OFFSET(12855, glProgramNamedParameter4fvNV, glProgramNamedParameter4fvNV, NULL, _gloffset_ProgramNamedParameter4fvNV),
    NAME_FUNC_OFFSET(12884, gl_dispatch_stub_744, gl_dispatch_stub_744, NULL, _gloffset_DepthBoundsEXT),
    NAME_FUNC_OFFSET(12901, gl_dispatch_stub_745, gl_dispatch_stub_745, NULL, _gloffset_BlendEquationSeparateEXT),
    NAME_FUNC_OFFSET(12928, glBindFramebufferEXT, glBindFramebufferEXT, NULL, _gloffset_BindFramebufferEXT),
    NAME_FUNC_OFFSET(12949, glBindRenderbufferEXT, glBindRenderbufferEXT, NULL, _gloffset_BindRenderbufferEXT),
    NAME_FUNC_OFFSET(12971, glCheckFramebufferStatusEXT, glCheckFramebufferStatusEXT, NULL, _gloffset_CheckFramebufferStatusEXT),
    NAME_FUNC_OFFSET(12999, glDeleteFramebuffersEXT, glDeleteFramebuffersEXT, NULL, _gloffset_DeleteFramebuffersEXT),
    NAME_FUNC_OFFSET(13023, glDeleteRenderbuffersEXT, glDeleteRenderbuffersEXT, NULL, _gloffset_DeleteRenderbuffersEXT),
    NAME_FUNC_OFFSET(13048, glFramebufferRenderbufferEXT, glFramebufferRenderbufferEXT, NULL, _gloffset_FramebufferRenderbufferEXT),
    NAME_FUNC_OFFSET(13077, glFramebufferTexture1DEXT, glFramebufferTexture1DEXT, NULL, _gloffset_FramebufferTexture1DEXT),
    NAME_FUNC_OFFSET(13103, glFramebufferTexture2DEXT, glFramebufferTexture2DEXT, NULL, _gloffset_FramebufferTexture2DEXT),
    NAME_FUNC_OFFSET(13129, glFramebufferTexture3DEXT, glFramebufferTexture3DEXT, NULL, _gloffset_FramebufferTexture3DEXT),
    NAME_FUNC_OFFSET(13155, glGenFramebuffersEXT, glGenFramebuffersEXT, NULL, _gloffset_GenFramebuffersEXT),
    NAME_FUNC_OFFSET(13176, glGenRenderbuffersEXT, glGenRenderbuffersEXT, NULL, _gloffset_GenRenderbuffersEXT),
    NAME_FUNC_OFFSET(13198, glGenerateMipmapEXT, glGenerateMipmapEXT, NULL, _gloffset_GenerateMipmapEXT),
    NAME_FUNC_OFFSET(13218, glGetFramebufferAttachmentParameterivEXT, glGetFramebufferAttachmentParameterivEXT, NULL, _gloffset_GetFramebufferAttachmentParameterivEXT),
    NAME_FUNC_OFFSET(13259, glGetRenderbufferParameterivEXT, glGetRenderbufferParameterivEXT, NULL, _gloffset_GetRenderbufferParameterivEXT),
    NAME_FUNC_OFFSET(13291, glIsFramebufferEXT, glIsFramebufferEXT, NULL, _gloffset_IsFramebufferEXT),
    NAME_FUNC_OFFSET(13310, glIsRenderbufferEXT, glIsRenderbufferEXT, NULL, _gloffset_IsRenderbufferEXT),
    NAME_FUNC_OFFSET(13330, glRenderbufferStorageEXT, glRenderbufferStorageEXT, NULL, _gloffset_RenderbufferStorageEXT),
    NAME_FUNC_OFFSET(13355, gl_dispatch_stub_763, gl_dispatch_stub_763, NULL, _gloffset_BlitFramebufferEXT),
    NAME_FUNC_OFFSET(13376, glFramebufferTextureLayerEXT, glFramebufferTextureLayerEXT, NULL, _gloffset_FramebufferTextureLayerEXT),
    NAME_FUNC_OFFSET(13405, gl_dispatch_stub_765, gl_dispatch_stub_765, NULL, _gloffset_StencilFuncSeparateATI),
    NAME_FUNC_OFFSET(13430, gl_dispatch_stub_766, gl_dispatch_stub_766, NULL, _gloffset_ProgramEnvParameters4fvEXT),
    NAME_FUNC_OFFSET(13459, gl_dispatch_stub_767, gl_dispatch_stub_767, NULL, _gloffset_ProgramLocalParameters4fvEXT),
    NAME_FUNC_OFFSET(13490, gl_dispatch_stub_768, gl_dispatch_stub_768, NULL, _gloffset_GetQueryObjecti64vEXT),
    NAME_FUNC_OFFSET(13514, gl_dispatch_stub_769, gl_dispatch_stub_769, NULL, _gloffset_GetQueryObjectui64vEXT),
    NAME_FUNC_OFFSET(13539, glArrayElement, glArrayElement, NULL, _gloffset_ArrayElement),
    NAME_FUNC_OFFSET(13557, glBindTexture, glBindTexture, NULL, _gloffset_BindTexture),
    NAME_FUNC_OFFSET(13574, glDrawArrays, glDrawArrays, NULL, _gloffset_DrawArrays),
    NAME_FUNC_OFFSET(13590, glAreTexturesResident, glAreTexturesResidentEXT, glAreTexturesResidentEXT, _gloffset_AreTexturesResident),
    NAME_FUNC_OFFSET(13615, glCopyTexImage1D, glCopyTexImage1D, NULL, _gloffset_CopyTexImage1D),
    NAME_FUNC_OFFSET(13635, glCopyTexImage2D, glCopyTexImage2D, NULL, _gloffset_CopyTexImage2D),
    NAME_FUNC_OFFSET(13655, glCopyTexSubImage1D, glCopyTexSubImage1D, NULL, _gloffset_CopyTexSubImage1D),
    NAME_FUNC_OFFSET(13678, glCopyTexSubImage2D, glCopyTexSubImage2D, NULL, _gloffset_CopyTexSubImage2D),
    NAME_FUNC_OFFSET(13701, glDeleteTextures, glDeleteTexturesEXT, glDeleteTexturesEXT, _gloffset_DeleteTextures),
    NAME_FUNC_OFFSET(13721, glGenTextures, glGenTexturesEXT, glGenTexturesEXT, _gloffset_GenTextures),
    NAME_FUNC_OFFSET(13738, glGetPointerv, glGetPointerv, NULL, _gloffset_GetPointerv),
    NAME_FUNC_OFFSET(13755, glIsTexture, glIsTextureEXT, glIsTextureEXT, _gloffset_IsTexture),
    NAME_FUNC_OFFSET(13770, glPrioritizeTextures, glPrioritizeTextures, NULL, _gloffset_PrioritizeTextures),
    NAME_FUNC_OFFSET(13794, glTexSubImage1D, glTexSubImage1D, NULL, _gloffset_TexSubImage1D),
    NAME_FUNC_OFFSET(13813, glTexSubImage2D, glTexSubImage2D, NULL, _gloffset_TexSubImage2D),
    NAME_FUNC_OFFSET(13832, glBlendColor, glBlendColor, NULL, _gloffset_BlendColor),
    NAME_FUNC_OFFSET(13848, glBlendEquation, glBlendEquation, NULL, _gloffset_BlendEquation),
    NAME_FUNC_OFFSET(13867, glDrawRangeElements, glDrawRangeElements, NULL, _gloffset_DrawRangeElements),
    NAME_FUNC_OFFSET(13890, glColorTable, glColorTable, NULL, _gloffset_ColorTable),
    NAME_FUNC_OFFSET(13906, glColorTable, glColorTable, NULL, _gloffset_ColorTable),
    NAME_FUNC_OFFSET(13922, glColorTableParameterfv, glColorTableParameterfv, NULL, _gloffset_ColorTableParameterfv),
    NAME_FUNC_OFFSET(13949, glColorTableParameteriv, glColorTableParameteriv, NULL, _gloffset_ColorTableParameteriv),
    NAME_FUNC_OFFSET(13976, glCopyColorTable, glCopyColorTable, NULL, _gloffset_CopyColorTable),
    NAME_FUNC_OFFSET(13996, glGetColorTable, glGetColorTableEXT, glGetColorTableEXT, _gloffset_GetColorTable),
    NAME_FUNC_OFFSET(14015, glGetColorTable, glGetColorTableEXT, glGetColorTableEXT, _gloffset_GetColorTable),
    NAME_FUNC_OFFSET(14034, glGetColorTableParameterfv, glGetColorTableParameterfvEXT, glGetColorTableParameterfvEXT, _gloffset_GetColorTableParameterfv),
    NAME_FUNC_OFFSET(14064, glGetColorTableParameterfv, glGetColorTableParameterfvEXT, glGetColorTableParameterfvEXT, _gloffset_GetColorTableParameterfv),
    NAME_FUNC_OFFSET(14094, glGetColorTableParameteriv, glGetColorTableParameterivEXT, glGetColorTableParameterivEXT, _gloffset_GetColorTableParameteriv),
    NAME_FUNC_OFFSET(14124, glGetColorTableParameteriv, glGetColorTableParameterivEXT, glGetColorTableParameterivEXT, _gloffset_GetColorTableParameteriv),
    NAME_FUNC_OFFSET(14154, glColorSubTable, glColorSubTable, NULL, _gloffset_ColorSubTable),
    NAME_FUNC_OFFSET(14173, glCopyColorSubTable, glCopyColorSubTable, NULL, _gloffset_CopyColorSubTable),
    NAME_FUNC_OFFSET(14196, glConvolutionFilter1D, glConvolutionFilter1D, NULL, _gloffset_ConvolutionFilter1D),
    NAME_FUNC_OFFSET(14221, glConvolutionFilter2D, glConvolutionFilter2D, NULL, _gloffset_ConvolutionFilter2D),
    NAME_FUNC_OFFSET(14246, glConvolutionParameterf, glConvolutionParameterf, NULL, _gloffset_ConvolutionParameterf),
    NAME_FUNC_OFFSET(14273, glConvolutionParameterfv, glConvolutionParameterfv, NULL, _gloffset_ConvolutionParameterfv),
    NAME_FUNC_OFFSET(14301, glConvolutionParameteri, glConvolutionParameteri, NULL, _gloffset_ConvolutionParameteri),
    NAME_FUNC_OFFSET(14328, glConvolutionParameteriv, glConvolutionParameteriv, NULL, _gloffset_ConvolutionParameteriv),
    NAME_FUNC_OFFSET(14356, glCopyConvolutionFilter1D, glCopyConvolutionFilter1D, NULL, _gloffset_CopyConvolutionFilter1D),
    NAME_FUNC_OFFSET(14385, glCopyConvolutionFilter2D, glCopyConvolutionFilter2D, NULL, _gloffset_CopyConvolutionFilter2D),
    NAME_FUNC_OFFSET(14414, glGetConvolutionFilter, gl_dispatch_stub_356, gl_dispatch_stub_356, _gloffset_GetConvolutionFilter),
    NAME_FUNC_OFFSET(14440, glGetConvolutionParameterfv, gl_dispatch_stub_357, gl_dispatch_stub_357, _gloffset_GetConvolutionParameterfv),
    NAME_FUNC_OFFSET(14471, glGetConvolutionParameteriv, gl_dispatch_stub_358, gl_dispatch_stub_358, _gloffset_GetConvolutionParameteriv),
    NAME_FUNC_OFFSET(14502, glGetSeparableFilter, gl_dispatch_stub_359, gl_dispatch_stub_359, _gloffset_GetSeparableFilter),
    NAME_FUNC_OFFSET(14526, glSeparableFilter2D, glSeparableFilter2D, NULL, _gloffset_SeparableFilter2D),
    NAME_FUNC_OFFSET(14549, glGetHistogram, gl_dispatch_stub_361, gl_dispatch_stub_361, _gloffset_GetHistogram),
    NAME_FUNC_OFFSET(14567, glGetHistogramParameterfv, gl_dispatch_stub_362, gl_dispatch_stub_362, _gloffset_GetHistogramParameterfv),
    NAME_FUNC_OFFSET(14596, glGetHistogramParameteriv, gl_dispatch_stub_363, gl_dispatch_stub_363, _gloffset_GetHistogramParameteriv),
    NAME_FUNC_OFFSET(14625, glGetMinmax, gl_dispatch_stub_364, gl_dispatch_stub_364, _gloffset_GetMinmax),
    NAME_FUNC_OFFSET(14640, glGetMinmaxParameterfv, gl_dispatch_stub_365, gl_dispatch_stub_365, _gloffset_GetMinmaxParameterfv),
    NAME_FUNC_OFFSET(14666, glGetMinmaxParameteriv, gl_dispatch_stub_366, gl_dispatch_stub_366, _gloffset_GetMinmaxParameteriv),
    NAME_FUNC_OFFSET(14692, glHistogram, glHistogram, NULL, _gloffset_Histogram),
    NAME_FUNC_OFFSET(14707, glMinmax, glMinmax, NULL, _gloffset_Minmax),
    NAME_FUNC_OFFSET(14719, glResetHistogram, glResetHistogram, NULL, _gloffset_ResetHistogram),
    NAME_FUNC_OFFSET(14739, glResetMinmax, glResetMinmax, NULL, _gloffset_ResetMinmax),
    NAME_FUNC_OFFSET(14756, glTexImage3D, glTexImage3D, NULL, _gloffset_TexImage3D),
    NAME_FUNC_OFFSET(14772, glTexSubImage3D, glTexSubImage3D, NULL, _gloffset_TexSubImage3D),
    NAME_FUNC_OFFSET(14791, glCopyTexSubImage3D, glCopyTexSubImage3D, NULL, _gloffset_CopyTexSubImage3D),
    NAME_FUNC_OFFSET(14814, glActiveTextureARB, glActiveTextureARB, NULL, _gloffset_ActiveTextureARB),
    NAME_FUNC_OFFSET(14830, glClientActiveTextureARB, glClientActiveTextureARB, NULL, _gloffset_ClientActiveTextureARB),
    NAME_FUNC_OFFSET(14852, glMultiTexCoord1dARB, glMultiTexCoord1dARB, NULL, _gloffset_MultiTexCoord1dARB),
    NAME_FUNC_OFFSET(14870, glMultiTexCoord1dvARB, glMultiTexCoord1dvARB, NULL, _gloffset_MultiTexCoord1dvARB),
    NAME_FUNC_OFFSET(14889, glMultiTexCoord1fARB, glMultiTexCoord1fARB, NULL, _gloffset_MultiTexCoord1fARB),
    NAME_FUNC_OFFSET(14907, glMultiTexCoord1fvARB, glMultiTexCoord1fvARB, NULL, _gloffset_MultiTexCoord1fvARB),
    NAME_FUNC_OFFSET(14926, glMultiTexCoord1iARB, glMultiTexCoord1iARB, NULL, _gloffset_MultiTexCoord1iARB),
    NAME_FUNC_OFFSET(14944, glMultiTexCoord1ivARB, glMultiTexCoord1ivARB, NULL, _gloffset_MultiTexCoord1ivARB),
    NAME_FUNC_OFFSET(14963, glMultiTexCoord1sARB, glMultiTexCoord1sARB, NULL, _gloffset_MultiTexCoord1sARB),
    NAME_FUNC_OFFSET(14981, glMultiTexCoord1svARB, glMultiTexCoord1svARB, NULL, _gloffset_MultiTexCoord1svARB),
    NAME_FUNC_OFFSET(15000, glMultiTexCoord2dARB, glMultiTexCoord2dARB, NULL, _gloffset_MultiTexCoord2dARB),
    NAME_FUNC_OFFSET(15018, glMultiTexCoord2dvARB, glMultiTexCoord2dvARB, NULL, _gloffset_MultiTexCoord2dvARB),
    NAME_FUNC_OFFSET(15037, glMultiTexCoord2fARB, glMultiTexCoord2fARB, NULL, _gloffset_MultiTexCoord2fARB),
    NAME_FUNC_OFFSET(15055, glMultiTexCoord2fvARB, glMultiTexCoord2fvARB, NULL, _gloffset_MultiTexCoord2fvARB),
    NAME_FUNC_OFFSET(15074, glMultiTexCoord2iARB, glMultiTexCoord2iARB, NULL, _gloffset_MultiTexCoord2iARB),
    NAME_FUNC_OFFSET(15092, glMultiTexCoord2ivARB, glMultiTexCoord2ivARB, NULL, _gloffset_MultiTexCoord2ivARB),
    NAME_FUNC_OFFSET(15111, glMultiTexCoord2sARB, glMultiTexCoord2sARB, NULL, _gloffset_MultiTexCoord2sARB),
    NAME_FUNC_OFFSET(15129, glMultiTexCoord2svARB, glMultiTexCoord2svARB, NULL, _gloffset_MultiTexCoord2svARB),
    NAME_FUNC_OFFSET(15148, glMultiTexCoord3dARB, glMultiTexCoord3dARB, NULL, _gloffset_MultiTexCoord3dARB),
    NAME_FUNC_OFFSET(15166, glMultiTexCoord3dvARB, glMultiTexCoord3dvARB, NULL, _gloffset_MultiTexCoord3dvARB),
    NAME_FUNC_OFFSET(15185, glMultiTexCoord3fARB, glMultiTexCoord3fARB, NULL, _gloffset_MultiTexCoord3fARB),
    NAME_FUNC_OFFSET(15203, glMultiTexCoord3fvARB, glMultiTexCoord3fvARB, NULL, _gloffset_MultiTexCoord3fvARB),
    NAME_FUNC_OFFSET(15222, glMultiTexCoord3iARB, glMultiTexCoord3iARB, NULL, _gloffset_MultiTexCoord3iARB),
    NAME_FUNC_OFFSET(15240, glMultiTexCoord3ivARB, glMultiTexCoord3ivARB, NULL, _gloffset_MultiTexCoord3ivARB),
    NAME_FUNC_OFFSET(15259, glMultiTexCoord3sARB, glMultiTexCoord3sARB, NULL, _gloffset_MultiTexCoord3sARB),
    NAME_FUNC_OFFSET(15277, glMultiTexCoord3svARB, glMultiTexCoord3svARB, NULL, _gloffset_MultiTexCoord3svARB),
    NAME_FUNC_OFFSET(15296, glMultiTexCoord4dARB, glMultiTexCoord4dARB, NULL, _gloffset_MultiTexCoord4dARB),
    NAME_FUNC_OFFSET(15314, glMultiTexCoord4dvARB, glMultiTexCoord4dvARB, NULL, _gloffset_MultiTexCoord4dvARB),
    NAME_FUNC_OFFSET(15333, glMultiTexCoord4fARB, glMultiTexCoord4fARB, NULL, _gloffset_MultiTexCoord4fARB),
    NAME_FUNC_OFFSET(15351, glMultiTexCoord4fvARB, glMultiTexCoord4fvARB, NULL, _gloffset_MultiTexCoord4fvARB),
    NAME_FUNC_OFFSET(15370, glMultiTexCoord4iARB, glMultiTexCoord4iARB, NULL, _gloffset_MultiTexCoord4iARB),
    NAME_FUNC_OFFSET(15388, glMultiTexCoord4ivARB, glMultiTexCoord4ivARB, NULL, _gloffset_MultiTexCoord4ivARB),
    NAME_FUNC_OFFSET(15407, glMultiTexCoord4sARB, glMultiTexCoord4sARB, NULL, _gloffset_MultiTexCoord4sARB),
    NAME_FUNC_OFFSET(15425, glMultiTexCoord4svARB, glMultiTexCoord4svARB, NULL, _gloffset_MultiTexCoord4svARB),
    NAME_FUNC_OFFSET(15444, glStencilOpSeparate, glStencilOpSeparate, NULL, _gloffset_StencilOpSeparate),
    NAME_FUNC_OFFSET(15467, glLoadTransposeMatrixdARB, glLoadTransposeMatrixdARB, NULL, _gloffset_LoadTransposeMatrixdARB),
    NAME_FUNC_OFFSET(15490, glLoadTransposeMatrixfARB, glLoadTransposeMatrixfARB, NULL, _gloffset_LoadTransposeMatrixfARB),
    NAME_FUNC_OFFSET(15513, glMultTransposeMatrixdARB, glMultTransposeMatrixdARB, NULL, _gloffset_MultTransposeMatrixdARB),
    NAME_FUNC_OFFSET(15536, glMultTransposeMatrixfARB, glMultTransposeMatrixfARB, NULL, _gloffset_MultTransposeMatrixfARB),
    NAME_FUNC_OFFSET(15559, glSampleCoverageARB, glSampleCoverageARB, NULL, _gloffset_SampleCoverageARB),
    NAME_FUNC_OFFSET(15576, glCompressedTexImage1DARB, glCompressedTexImage1DARB, NULL, _gloffset_CompressedTexImage1DARB),
    NAME_FUNC_OFFSET(15599, glCompressedTexImage2DARB, glCompressedTexImage2DARB, NULL, _gloffset_CompressedTexImage2DARB),
    NAME_FUNC_OFFSET(15622, glCompressedTexImage3DARB, glCompressedTexImage3DARB, NULL, _gloffset_CompressedTexImage3DARB),
    NAME_FUNC_OFFSET(15645, glCompressedTexSubImage1DARB, glCompressedTexSubImage1DARB, NULL, _gloffset_CompressedTexSubImage1DARB),
    NAME_FUNC_OFFSET(15671, glCompressedTexSubImage2DARB, glCompressedTexSubImage2DARB, NULL, _gloffset_CompressedTexSubImage2DARB),
    NAME_FUNC_OFFSET(15697, glCompressedTexSubImage3DARB, glCompressedTexSubImage3DARB, NULL, _gloffset_CompressedTexSubImage3DARB),
    NAME_FUNC_OFFSET(15723, glGetCompressedTexImageARB, glGetCompressedTexImageARB, NULL, _gloffset_GetCompressedTexImageARB),
    NAME_FUNC_OFFSET(15747, glDisableVertexAttribArrayARB, glDisableVertexAttribArrayARB, NULL, _gloffset_DisableVertexAttribArrayARB),
    NAME_FUNC_OFFSET(15774, glEnableVertexAttribArrayARB, glEnableVertexAttribArrayARB, NULL, _gloffset_EnableVertexAttribArrayARB),
    NAME_FUNC_OFFSET(15800, glGetVertexAttribdvARB, glGetVertexAttribdvARB, NULL, _gloffset_GetVertexAttribdvARB),
    NAME_FUNC_OFFSET(15820, glGetVertexAttribfvARB, glGetVertexAttribfvARB, NULL, _gloffset_GetVertexAttribfvARB),
    NAME_FUNC_OFFSET(15840, glGetVertexAttribivARB, glGetVertexAttribivARB, NULL, _gloffset_GetVertexAttribivARB),
    NAME_FUNC_OFFSET(15860, glProgramEnvParameter4dARB, glProgramEnvParameter4dARB, NULL, _gloffset_ProgramEnvParameter4dARB),
    NAME_FUNC_OFFSET(15883, glProgramEnvParameter4dvARB, glProgramEnvParameter4dvARB, NULL, _gloffset_ProgramEnvParameter4dvARB),
    NAME_FUNC_OFFSET(15907, glProgramEnvParameter4fARB, glProgramEnvParameter4fARB, NULL, _gloffset_ProgramEnvParameter4fARB),
    NAME_FUNC_OFFSET(15930, glProgramEnvParameter4fvARB, glProgramEnvParameter4fvARB, NULL, _gloffset_ProgramEnvParameter4fvARB),
    NAME_FUNC_OFFSET(15954, glVertexAttrib1dARB, glVertexAttrib1dARB, NULL, _gloffset_VertexAttrib1dARB),
    NAME_FUNC_OFFSET(15971, glVertexAttrib1dvARB, glVertexAttrib1dvARB, NULL, _gloffset_VertexAttrib1dvARB),
    NAME_FUNC_OFFSET(15989, glVertexAttrib1fARB, glVertexAttrib1fARB, NULL, _gloffset_VertexAttrib1fARB),
    NAME_FUNC_OFFSET(16006, glVertexAttrib1fvARB, glVertexAttrib1fvARB, NULL, _gloffset_VertexAttrib1fvARB),
    NAME_FUNC_OFFSET(16024, glVertexAttrib1sARB, glVertexAttrib1sARB, NULL, _gloffset_VertexAttrib1sARB),
    NAME_FUNC_OFFSET(16041, glVertexAttrib1svARB, glVertexAttrib1svARB, NULL, _gloffset_VertexAttrib1svARB),
    NAME_FUNC_OFFSET(16059, glVertexAttrib2dARB, glVertexAttrib2dARB, NULL, _gloffset_VertexAttrib2dARB),
    NAME_FUNC_OFFSET(16076, glVertexAttrib2dvARB, glVertexAttrib2dvARB, NULL, _gloffset_VertexAttrib2dvARB),
    NAME_FUNC_OFFSET(16094, glVertexAttrib2fARB, glVertexAttrib2fARB, NULL, _gloffset_VertexAttrib2fARB),
    NAME_FUNC_OFFSET(16111, glVertexAttrib2fvARB, glVertexAttrib2fvARB, NULL, _gloffset_VertexAttrib2fvARB),
    NAME_FUNC_OFFSET(16129, glVertexAttrib2sARB, glVertexAttrib2sARB, NULL, _gloffset_VertexAttrib2sARB),
    NAME_FUNC_OFFSET(16146, glVertexAttrib2svARB, glVertexAttrib2svARB, NULL, _gloffset_VertexAttrib2svARB),
    NAME_FUNC_OFFSET(16164, glVertexAttrib3dARB, glVertexAttrib3dARB, NULL, _gloffset_VertexAttrib3dARB),
    NAME_FUNC_OFFSET(16181, glVertexAttrib3dvARB, glVertexAttrib3dvARB, NULL, _gloffset_VertexAttrib3dvARB),
    NAME_FUNC_OFFSET(16199, glVertexAttrib3fARB, glVertexAttrib3fARB, NULL, _gloffset_VertexAttrib3fARB),
    NAME_FUNC_OFFSET(16216, glVertexAttrib3fvARB, glVertexAttrib3fvARB, NULL, _gloffset_VertexAttrib3fvARB),
    NAME_FUNC_OFFSET(16234, glVertexAttrib3sARB, glVertexAttrib3sARB, NULL, _gloffset_VertexAttrib3sARB),
    NAME_FUNC_OFFSET(16251, glVertexAttrib3svARB, glVertexAttrib3svARB, NULL, _gloffset_VertexAttrib3svARB),
    NAME_FUNC_OFFSET(16269, glVertexAttrib4NbvARB, glVertexAttrib4NbvARB, NULL, _gloffset_VertexAttrib4NbvARB),
    NAME_FUNC_OFFSET(16288, glVertexAttrib4NivARB, glVertexAttrib4NivARB, NULL, _gloffset_VertexAttrib4NivARB),
    NAME_FUNC_OFFSET(16307, glVertexAttrib4NsvARB, glVertexAttrib4NsvARB, NULL, _gloffset_VertexAttrib4NsvARB),
    NAME_FUNC_OFFSET(16326, glVertexAttrib4NubARB, glVertexAttrib4NubARB, NULL, _gloffset_VertexAttrib4NubARB),
    NAME_FUNC_OFFSET(16345, glVertexAttrib4NubvARB, glVertexAttrib4NubvARB, NULL, _gloffset_VertexAttrib4NubvARB),
    NAME_FUNC_OFFSET(16365, glVertexAttrib4NuivARB, glVertexAttrib4NuivARB, NULL, _gloffset_VertexAttrib4NuivARB),
    NAME_FUNC_OFFSET(16385, glVertexAttrib4NusvARB, glVertexAttrib4NusvARB, NULL, _gloffset_VertexAttrib4NusvARB),
    NAME_FUNC_OFFSET(16405, glVertexAttrib4bvARB, glVertexAttrib4bvARB, NULL, _gloffset_VertexAttrib4bvARB),
    NAME_FUNC_OFFSET(16423, glVertexAttrib4dARB, glVertexAttrib4dARB, NULL, _gloffset_VertexAttrib4dARB),
    NAME_FUNC_OFFSET(16440, glVertexAttrib4dvARB, glVertexAttrib4dvARB, NULL, _gloffset_VertexAttrib4dvARB),
    NAME_FUNC_OFFSET(16458, glVertexAttrib4fARB, glVertexAttrib4fARB, NULL, _gloffset_VertexAttrib4fARB),
    NAME_FUNC_OFFSET(16475, glVertexAttrib4fvARB, glVertexAttrib4fvARB, NULL, _gloffset_VertexAttrib4fvARB),
    NAME_FUNC_OFFSET(16493, glVertexAttrib4ivARB, glVertexAttrib4ivARB, NULL, _gloffset_VertexAttrib4ivARB),
    NAME_FUNC_OFFSET(16511, glVertexAttrib4sARB, glVertexAttrib4sARB, NULL, _gloffset_VertexAttrib4sARB),
    NAME_FUNC_OFFSET(16528, glVertexAttrib4svARB, glVertexAttrib4svARB, NULL, _gloffset_VertexAttrib4svARB),
    NAME_FUNC_OFFSET(16546, glVertexAttrib4ubvARB, glVertexAttrib4ubvARB, NULL, _gloffset_VertexAttrib4ubvARB),
    NAME_FUNC_OFFSET(16565, glVertexAttrib4uivARB, glVertexAttrib4uivARB, NULL, _gloffset_VertexAttrib4uivARB),
    NAME_FUNC_OFFSET(16584, glVertexAttrib4usvARB, glVertexAttrib4usvARB, NULL, _gloffset_VertexAttrib4usvARB),
    NAME_FUNC_OFFSET(16603, glVertexAttribPointerARB, glVertexAttribPointerARB, NULL, _gloffset_VertexAttribPointerARB),
    NAME_FUNC_OFFSET(16625, glBindBufferARB, glBindBufferARB, NULL, _gloffset_BindBufferARB),
    NAME_FUNC_OFFSET(16638, glBufferDataARB, glBufferDataARB, NULL, _gloffset_BufferDataARB),
    NAME_FUNC_OFFSET(16651, glBufferSubDataARB, glBufferSubDataARB, NULL, _gloffset_BufferSubDataARB),
    NAME_FUNC_OFFSET(16667, glDeleteBuffersARB, glDeleteBuffersARB, NULL, _gloffset_DeleteBuffersARB),
    NAME_FUNC_OFFSET(16683, glGenBuffersARB, glGenBuffersARB, NULL, _gloffset_GenBuffersARB),
    NAME_FUNC_OFFSET(16696, glGetBufferParameterivARB, glGetBufferParameterivARB, NULL, _gloffset_GetBufferParameterivARB),
    NAME_FUNC_OFFSET(16719, glGetBufferPointervARB, glGetBufferPointervARB, NULL, _gloffset_GetBufferPointervARB),
    NAME_FUNC_OFFSET(16739, glGetBufferSubDataARB, glGetBufferSubDataARB, NULL, _gloffset_GetBufferSubDataARB),
    NAME_FUNC_OFFSET(16758, glIsBufferARB, glIsBufferARB, NULL, _gloffset_IsBufferARB),
    NAME_FUNC_OFFSET(16769, glMapBufferARB, glMapBufferARB, NULL, _gloffset_MapBufferARB),
    NAME_FUNC_OFFSET(16781, glUnmapBufferARB, glUnmapBufferARB, NULL, _gloffset_UnmapBufferARB),
    NAME_FUNC_OFFSET(16795, glBeginQueryARB, glBeginQueryARB, NULL, _gloffset_BeginQueryARB),
    NAME_FUNC_OFFSET(16808, glDeleteQueriesARB, glDeleteQueriesARB, NULL, _gloffset_DeleteQueriesARB),
    NAME_FUNC_OFFSET(16824, glEndQueryARB, glEndQueryARB, NULL, _gloffset_EndQueryARB),
    NAME_FUNC_OFFSET(16835, glGenQueriesARB, glGenQueriesARB, NULL, _gloffset_GenQueriesARB),
    NAME_FUNC_OFFSET(16848, glGetQueryObjectivARB, glGetQueryObjectivARB, NULL, _gloffset_GetQueryObjectivARB),
    NAME_FUNC_OFFSET(16867, glGetQueryObjectuivARB, glGetQueryObjectuivARB, NULL, _gloffset_GetQueryObjectuivARB),
    NAME_FUNC_OFFSET(16887, glGetQueryivARB, glGetQueryivARB, NULL, _gloffset_GetQueryivARB),
    NAME_FUNC_OFFSET(16900, glIsQueryARB, glIsQueryARB, NULL, _gloffset_IsQueryARB),
    NAME_FUNC_OFFSET(16910, glCompileShaderARB, glCompileShaderARB, NULL, _gloffset_CompileShaderARB),
    NAME_FUNC_OFFSET(16926, glGetActiveUniformARB, glGetActiveUniformARB, NULL, _gloffset_GetActiveUniformARB),
    NAME_FUNC_OFFSET(16945, glGetShaderSourceARB, glGetShaderSourceARB, NULL, _gloffset_GetShaderSourceARB),
    NAME_FUNC_OFFSET(16963, glGetUniformLocationARB, glGetUniformLocationARB, NULL, _gloffset_GetUniformLocationARB),
    NAME_FUNC_OFFSET(16984, glGetUniformfvARB, glGetUniformfvARB, NULL, _gloffset_GetUniformfvARB),
    NAME_FUNC_OFFSET(16999, glGetUniformivARB, glGetUniformivARB, NULL, _gloffset_GetUniformivARB),
    NAME_FUNC_OFFSET(17014, glLinkProgramARB, glLinkProgramARB, NULL, _gloffset_LinkProgramARB),
    NAME_FUNC_OFFSET(17028, glShaderSourceARB, glShaderSourceARB, NULL, _gloffset_ShaderSourceARB),
    NAME_FUNC_OFFSET(17043, glUniform1fARB, glUniform1fARB, NULL, _gloffset_Uniform1fARB),
    NAME_FUNC_OFFSET(17055, glUniform1fvARB, glUniform1fvARB, NULL, _gloffset_Uniform1fvARB),
    NAME_FUNC_OFFSET(17068, glUniform1iARB, glUniform1iARB, NULL, _gloffset_Uniform1iARB),
    NAME_FUNC_OFFSET(17080, glUniform1ivARB, glUniform1ivARB, NULL, _gloffset_Uniform1ivARB),
    NAME_FUNC_OFFSET(17093, glUniform2fARB, glUniform2fARB, NULL, _gloffset_Uniform2fARB),
    NAME_FUNC_OFFSET(17105, glUniform2fvARB, glUniform2fvARB, NULL, _gloffset_Uniform2fvARB),
    NAME_FUNC_OFFSET(17118, glUniform2iARB, glUniform2iARB, NULL, _gloffset_Uniform2iARB),
    NAME_FUNC_OFFSET(17130, glUniform2ivARB, glUniform2ivARB, NULL, _gloffset_Uniform2ivARB),
    NAME_FUNC_OFFSET(17143, glUniform3fARB, glUniform3fARB, NULL, _gloffset_Uniform3fARB),
    NAME_FUNC_OFFSET(17155, glUniform3fvARB, glUniform3fvARB, NULL, _gloffset_Uniform3fvARB),
    NAME_FUNC_OFFSET(17168, glUniform3iARB, glUniform3iARB, NULL, _gloffset_Uniform3iARB),
    NAME_FUNC_OFFSET(17180, glUniform3ivARB, glUniform3ivARB, NULL, _gloffset_Uniform3ivARB),
    NAME_FUNC_OFFSET(17193, glUniform4fARB, glUniform4fARB, NULL, _gloffset_Uniform4fARB),
    NAME_FUNC_OFFSET(17205, glUniform4fvARB, glUniform4fvARB, NULL, _gloffset_Uniform4fvARB),
    NAME_FUNC_OFFSET(17218, glUniform4iARB, glUniform4iARB, NULL, _gloffset_Uniform4iARB),
    NAME_FUNC_OFFSET(17230, glUniform4ivARB, glUniform4ivARB, NULL, _gloffset_Uniform4ivARB),
    NAME_FUNC_OFFSET(17243, glUniformMatrix2fvARB, glUniformMatrix2fvARB, NULL, _gloffset_UniformMatrix2fvARB),
    NAME_FUNC_OFFSET(17262, glUniformMatrix3fvARB, glUniformMatrix3fvARB, NULL, _gloffset_UniformMatrix3fvARB),
    NAME_FUNC_OFFSET(17281, glUniformMatrix4fvARB, glUniformMatrix4fvARB, NULL, _gloffset_UniformMatrix4fvARB),
    NAME_FUNC_OFFSET(17300, glUseProgramObjectARB, glUseProgramObjectARB, NULL, _gloffset_UseProgramObjectARB),
    NAME_FUNC_OFFSET(17313, glValidateProgramARB, glValidateProgramARB, NULL, _gloffset_ValidateProgramARB),
    NAME_FUNC_OFFSET(17331, glBindAttribLocationARB, glBindAttribLocationARB, NULL, _gloffset_BindAttribLocationARB),
    NAME_FUNC_OFFSET(17352, glGetActiveAttribARB, glGetActiveAttribARB, NULL, _gloffset_GetActiveAttribARB),
    NAME_FUNC_OFFSET(17370, glGetAttribLocationARB, glGetAttribLocationARB, NULL, _gloffset_GetAttribLocationARB),
    NAME_FUNC_OFFSET(17390, glDrawBuffersARB, glDrawBuffersARB, NULL, _gloffset_DrawBuffersARB),
    NAME_FUNC_OFFSET(17404, glDrawBuffersARB, glDrawBuffersARB, NULL, _gloffset_DrawBuffersARB),
    NAME_FUNC_OFFSET(17421, gl_dispatch_stub_568, gl_dispatch_stub_568, NULL, _gloffset_SampleMaskSGIS),
    NAME_FUNC_OFFSET(17437, gl_dispatch_stub_569, gl_dispatch_stub_569, NULL, _gloffset_SamplePatternSGIS),
    NAME_FUNC_OFFSET(17456, glPointParameterfEXT, glPointParameterfEXT, NULL, _gloffset_PointParameterfEXT),
    NAME_FUNC_OFFSET(17474, glPointParameterfEXT, glPointParameterfEXT, NULL, _gloffset_PointParameterfEXT),
    NAME_FUNC_OFFSET(17495, glPointParameterfEXT, glPointParameterfEXT, NULL, _gloffset_PointParameterfEXT),
    NAME_FUNC_OFFSET(17517, glPointParameterfvEXT, glPointParameterfvEXT, NULL, _gloffset_PointParameterfvEXT),
    NAME_FUNC_OFFSET(17536, glPointParameterfvEXT, glPointParameterfvEXT, NULL, _gloffset_PointParameterfvEXT),
    NAME_FUNC_OFFSET(17558, glPointParameterfvEXT, glPointParameterfvEXT, NULL, _gloffset_PointParameterfvEXT),
    NAME_FUNC_OFFSET(17581, glSecondaryColor3bEXT, glSecondaryColor3bEXT, NULL, _gloffset_SecondaryColor3bEXT),
    NAME_FUNC_OFFSET(17600, glSecondaryColor3bvEXT, glSecondaryColor3bvEXT, NULL, _gloffset_SecondaryColor3bvEXT),
    NAME_FUNC_OFFSET(17620, glSecondaryColor3dEXT, glSecondaryColor3dEXT, NULL, _gloffset_SecondaryColor3dEXT),
    NAME_FUNC_OFFSET(17639, glSecondaryColor3dvEXT, glSecondaryColor3dvEXT, NULL, _gloffset_SecondaryColor3dvEXT),
    NAME_FUNC_OFFSET(17659, glSecondaryColor3fEXT, glSecondaryColor3fEXT, NULL, _gloffset_SecondaryColor3fEXT),
    NAME_FUNC_OFFSET(17678, glSecondaryColor3fvEXT, glSecondaryColor3fvEXT, NULL, _gloffset_SecondaryColor3fvEXT),
    NAME_FUNC_OFFSET(17698, glSecondaryColor3iEXT, glSecondaryColor3iEXT, NULL, _gloffset_SecondaryColor3iEXT),
    NAME_FUNC_OFFSET(17717, glSecondaryColor3ivEXT, glSecondaryColor3ivEXT, NULL, _gloffset_SecondaryColor3ivEXT),
    NAME_FUNC_OFFSET(17737, glSecondaryColor3sEXT, glSecondaryColor3sEXT, NULL, _gloffset_SecondaryColor3sEXT),
    NAME_FUNC_OFFSET(17756, glSecondaryColor3svEXT, glSecondaryColor3svEXT, NULL, _gloffset_SecondaryColor3svEXT),
    NAME_FUNC_OFFSET(17776, glSecondaryColor3ubEXT, glSecondaryColor3ubEXT, NULL, _gloffset_SecondaryColor3ubEXT),
    NAME_FUNC_OFFSET(17796, glSecondaryColor3ubvEXT, glSecondaryColor3ubvEXT, NULL, _gloffset_SecondaryColor3ubvEXT),
    NAME_FUNC_OFFSET(17817, glSecondaryColor3uiEXT, glSecondaryColor3uiEXT, NULL, _gloffset_SecondaryColor3uiEXT),
    NAME_FUNC_OFFSET(17837, glSecondaryColor3uivEXT, glSecondaryColor3uivEXT, NULL, _gloffset_SecondaryColor3uivEXT),
    NAME_FUNC_OFFSET(17858, glSecondaryColor3usEXT, glSecondaryColor3usEXT, NULL, _gloffset_SecondaryColor3usEXT),
    NAME_FUNC_OFFSET(17878, glSecondaryColor3usvEXT, glSecondaryColor3usvEXT, NULL, _gloffset_SecondaryColor3usvEXT),
    NAME_FUNC_OFFSET(17899, glSecondaryColorPointerEXT, glSecondaryColorPointerEXT, NULL, _gloffset_SecondaryColorPointerEXT),
    NAME_FUNC_OFFSET(17923, glMultiDrawArraysEXT, glMultiDrawArraysEXT, NULL, _gloffset_MultiDrawArraysEXT),
    NAME_FUNC_OFFSET(17941, glMultiDrawElementsEXT, glMultiDrawElementsEXT, NULL, _gloffset_MultiDrawElementsEXT),
    NAME_FUNC_OFFSET(17961, glFogCoordPointerEXT, glFogCoordPointerEXT, NULL, _gloffset_FogCoordPointerEXT),
    NAME_FUNC_OFFSET(17979, glFogCoorddEXT, glFogCoorddEXT, NULL, _gloffset_FogCoorddEXT),
    NAME_FUNC_OFFSET(17991, glFogCoorddvEXT, glFogCoorddvEXT, NULL, _gloffset_FogCoorddvEXT),
    NAME_FUNC_OFFSET(18004, glFogCoordfEXT, glFogCoordfEXT, NULL, _gloffset_FogCoordfEXT),
    NAME_FUNC_OFFSET(18016, glFogCoordfvEXT, glFogCoordfvEXT, NULL, _gloffset_FogCoordfvEXT),
    NAME_FUNC_OFFSET(18029, glBlendFuncSeparateEXT, glBlendFuncSeparateEXT, NULL, _gloffset_BlendFuncSeparateEXT),
    NAME_FUNC_OFFSET(18049, glBlendFuncSeparateEXT, glBlendFuncSeparateEXT, NULL, _gloffset_BlendFuncSeparateEXT),
    NAME_FUNC_OFFSET(18073, glWindowPos2dMESA, glWindowPos2dMESA, NULL, _gloffset_WindowPos2dMESA),
    NAME_FUNC_OFFSET(18087, glWindowPos2dMESA, glWindowPos2dMESA, NULL, _gloffset_WindowPos2dMESA),
    NAME_FUNC_OFFSET(18104, glWindowPos2dvMESA, glWindowPos2dvMESA, NULL, _gloffset_WindowPos2dvMESA),
    NAME_FUNC_OFFSET(18119, glWindowPos2dvMESA, glWindowPos2dvMESA, NULL, _gloffset_WindowPos2dvMESA),
    NAME_FUNC_OFFSET(18137, glWindowPos2fMESA, glWindowPos2fMESA, NULL, _gloffset_WindowPos2fMESA),
    NAME_FUNC_OFFSET(18151, glWindowPos2fMESA, glWindowPos2fMESA, NULL, _gloffset_WindowPos2fMESA),
    NAME_FUNC_OFFSET(18168, glWindowPos2fvMESA, glWindowPos2fvMESA, NULL, _gloffset_WindowPos2fvMESA),
    NAME_FUNC_OFFSET(18183, glWindowPos2fvMESA, glWindowPos2fvMESA, NULL, _gloffset_WindowPos2fvMESA),
    NAME_FUNC_OFFSET(18201, glWindowPos2iMESA, glWindowPos2iMESA, NULL, _gloffset_WindowPos2iMESA),
    NAME_FUNC_OFFSET(18215, glWindowPos2iMESA, glWindowPos2iMESA, NULL, _gloffset_WindowPos2iMESA),
    NAME_FUNC_OFFSET(18232, glWindowPos2ivMESA, glWindowPos2ivMESA, NULL, _gloffset_WindowPos2ivMESA),
    NAME_FUNC_OFFSET(18247, glWindowPos2ivMESA, glWindowPos2ivMESA, NULL, _gloffset_WindowPos2ivMESA),
    NAME_FUNC_OFFSET(18265, glWindowPos2sMESA, glWindowPos2sMESA, NULL, _gloffset_WindowPos2sMESA),
    NAME_FUNC_OFFSET(18279, glWindowPos2sMESA, glWindowPos2sMESA, NULL, _gloffset_WindowPos2sMESA),
    NAME_FUNC_OFFSET(18296, glWindowPos2svMESA, glWindowPos2svMESA, NULL, _gloffset_WindowPos2svMESA),
    NAME_FUNC_OFFSET(18311, glWindowPos2svMESA, glWindowPos2svMESA, NULL, _gloffset_WindowPos2svMESA),
    NAME_FUNC_OFFSET(18329, glWindowPos3dMESA, glWindowPos3dMESA, NULL, _gloffset_WindowPos3dMESA),
    NAME_FUNC_OFFSET(18343, glWindowPos3dMESA, glWindowPos3dMESA, NULL, _gloffset_WindowPos3dMESA),
    NAME_FUNC_OFFSET(18360, glWindowPos3dvMESA, glWindowPos3dvMESA, NULL, _gloffset_WindowPos3dvMESA),
    NAME_FUNC_OFFSET(18375, glWindowPos3dvMESA, glWindowPos3dvMESA, NULL, _gloffset_WindowPos3dvMESA),
    NAME_FUNC_OFFSET(18393, glWindowPos3fMESA, glWindowPos3fMESA, NULL, _gloffset_WindowPos3fMESA),
    NAME_FUNC_OFFSET(18407, glWindowPos3fMESA, glWindowPos3fMESA, NULL, _gloffset_WindowPos3fMESA),
    NAME_FUNC_OFFSET(18424, glWindowPos3fvMESA, glWindowPos3fvMESA, NULL, _gloffset_WindowPos3fvMESA),
    NAME_FUNC_OFFSET(18439, glWindowPos3fvMESA, glWindowPos3fvMESA, NULL, _gloffset_WindowPos3fvMESA),
    NAME_FUNC_OFFSET(18457, glWindowPos3iMESA, glWindowPos3iMESA, NULL, _gloffset_WindowPos3iMESA),
    NAME_FUNC_OFFSET(18471, glWindowPos3iMESA, glWindowPos3iMESA, NULL, _gloffset_WindowPos3iMESA),
    NAME_FUNC_OFFSET(18488, glWindowPos3ivMESA, glWindowPos3ivMESA, NULL, _gloffset_WindowPos3ivMESA),
    NAME_FUNC_OFFSET(18503, glWindowPos3ivMESA, glWindowPos3ivMESA, NULL, _gloffset_WindowPos3ivMESA),
    NAME_FUNC_OFFSET(18521, glWindowPos3sMESA, glWindowPos3sMESA, NULL, _gloffset_WindowPos3sMESA),
    NAME_FUNC_OFFSET(18535, glWindowPos3sMESA, glWindowPos3sMESA, NULL, _gloffset_WindowPos3sMESA),
    NAME_FUNC_OFFSET(18552, glWindowPos3svMESA, glWindowPos3svMESA, NULL, _gloffset_WindowPos3svMESA),
    NAME_FUNC_OFFSET(18567, glWindowPos3svMESA, glWindowPos3svMESA, NULL, _gloffset_WindowPos3svMESA),
    NAME_FUNC_OFFSET(18585, glBindProgramNV, glBindProgramNV, NULL, _gloffset_BindProgramNV),
    NAME_FUNC_OFFSET(18602, glDeleteProgramsNV, glDeleteProgramsNV, NULL, _gloffset_DeleteProgramsNV),
    NAME_FUNC_OFFSET(18622, glGenProgramsNV, glGenProgramsNV, NULL, _gloffset_GenProgramsNV),
    NAME_FUNC_OFFSET(18639, glGetVertexAttribPointervNV, glGetVertexAttribPointervNV, NULL, _gloffset_GetVertexAttribPointervNV),
    NAME_FUNC_OFFSET(18665, glGetVertexAttribPointervNV, glGetVertexAttribPointervNV, NULL, _gloffset_GetVertexAttribPointervNV),
    NAME_FUNC_OFFSET(18694, glIsProgramNV, glIsProgramNV, NULL, _gloffset_IsProgramNV),
    NAME_FUNC_OFFSET(18709, glPointParameteriNV, glPointParameteriNV, NULL, _gloffset_PointParameteriNV),
    NAME_FUNC_OFFSET(18727, glPointParameterivNV, glPointParameterivNV, NULL, _gloffset_PointParameterivNV),
    NAME_FUNC_OFFSET(18746, gl_dispatch_stub_745, gl_dispatch_stub_745, NULL, _gloffset_BlendEquationSeparateEXT),
    NAME_FUNC_OFFSET(18770, gl_dispatch_stub_745, gl_dispatch_stub_745, NULL, _gloffset_BlendEquationSeparateEXT),
    NAME_FUNC_OFFSET(-1, NULL, NULL, NULL, 0)
};

#undef NAME_FUNC_OFFSET
