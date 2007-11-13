/*
 * Mesa 3-D graphics library
 * Version:  6.5.2
 *
 * Copyright (C) 1999-2006  Brian Paul   All Rights Reserved.
 *
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
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


#ifndef ARBPROGRAM_H
#define ARBPROGRAM_H


extern void GLAPIENTRY
_mesa_EnableVertexAttribArrayARB(GLuint index);


extern void GLAPIENTRY
_mesa_DisableVertexAttribArrayARB(GLuint index);


extern void GLAPIENTRY
_mesa_GetVertexAttribdvARB(GLuint index, GLenum pname, GLdouble *params);


extern void GLAPIENTRY
_mesa_GetVertexAttribfvARB(GLuint index, GLenum pname, GLfloat *params);


extern void GLAPIENTRY
_mesa_GetVertexAttribivARB(GLuint index, GLenum pname, GLint *params);


extern void GLAPIENTRY
_mesa_GetVertexAttribPointervARB(GLuint index, GLenum pname, GLvoid **pointer);


extern GLboolean GLAPIENTRY
_mesa_IsProgramARB(GLuint id);


extern void GLAPIENTRY
_mesa_ProgramStringARB(GLenum target, GLenum format, GLsizei len,
                       const GLvoid *string);


extern void GLAPIENTRY
_mesa_ProgramEnvParameter4dARB(GLenum target, GLuint index,
                               GLdouble x, GLdouble y, GLdouble z, GLdouble w);


extern void GLAPIENTRY
_mesa_ProgramEnvParameter4dvARB(GLenum target, GLuint index,
                                const GLdouble *params);


extern void GLAPIENTRY
_mesa_ProgramEnvParameter4fARB(GLenum target, GLuint index,
                               GLfloat x, GLfloat y, GLfloat z, GLfloat w);


extern void GLAPIENTRY
_mesa_ProgramEnvParameter4fvARB(GLenum target, GLuint index,
                                const GLfloat *params);


extern void GLAPIENTRY
_mesa_ProgramEnvParameters4fvEXT(GLenum target, GLuint index, GLsizei count,
				 const GLfloat *params);


extern void GLAPIENTRY
_mesa_ProgramLocalParameter4dARB(GLenum target, GLuint index,
                                 GLdouble x, GLdouble y,
                                 GLdouble z, GLdouble w);


extern void GLAPIENTRY
_mesa_ProgramLocalParameter4dvARB(GLenum target, GLuint index,
                                  const GLdouble *params);


extern void GLAPIENTRY
_mesa_ProgramLocalParameter4fARB(GLenum target, GLuint index,
                                 GLfloat x, GLfloat y, GLfloat z, GLfloat w);


extern void GLAPIENTRY
_mesa_ProgramLocalParameter4fvARB(GLenum target, GLuint index,
                                  const GLfloat *params);


extern void GLAPIENTRY
_mesa_ProgramLocalParameters4fvEXT(GLenum target, GLuint index, GLsizei count,
				   const GLfloat *params);


extern void GLAPIENTRY
_mesa_GetProgramEnvParameterdvARB(GLenum target, GLuint index,
                                  GLdouble *params);


extern void GLAPIENTRY
_mesa_GetProgramEnvParameterfvARB(GLenum target, GLuint index, 
                                  GLfloat *params);


extern void GLAPIENTRY
_mesa_GetProgramLocalParameterdvARB(GLenum target, GLuint index,
                                    GLdouble *params);


extern void GLAPIENTRY
_mesa_GetProgramLocalParameterfvARB(GLenum target, GLuint index, 
                                    GLfloat *params);


extern void GLAPIENTRY
_mesa_GetProgramivARB(GLenum target, GLenum pname, GLint *params);


extern void GLAPIENTRY
_mesa_GetProgramStringARB(GLenum target, GLenum pname, GLvoid *string);


#endif
