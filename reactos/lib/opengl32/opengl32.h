/* $Id: opengl32.h,v 1.1 2004/02/01 07:11:06 royce Exp $
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS kernel
 * FILE:                 lib/opengl32/opengl32.h
 * PURPOSE:              OpenGL32 lib
 * PROGRAMMER:           Royce Mitchell III
 * UPDATE HISTORY:
 *                       Feb 1, 2004: Created
 */

#ifndef OPENGL32_PRIVATE_H
#define OPENGL32_PRIVATE_H

typedef
void
WINAPI
(*PGLACCUM) (
	GLenum op,
	GLfloat value );

typedef
void
WINAPI
(*PGLADDSWAPHINTRECTWIN) (
	GLint x,
	GLint y,
	GLsizei width,
	GLsizei height );

typedef
void
WINAPI
(*PGLARRAYELEMENT) ( GLint index );

typedef
void
WINAPI
(*PGLBEGIN) ( GLenum mode );

typedef
void
WINAPI
(*PGLBINDTEXTURE) ( GLenum target, GLuint texture )


typedef
void
WINAPI
(*PGLEND) ( void );

typedef struct tagGLFUNCLIST
{
	PGLACCUM glAccum;
	PGLADDSWAPHINTRECTWIN glAddSwapHintRectWIN;
	PGLARRAYELEMENT glArrayElement;
	PGLBEGIN glBegin;
	PGLBINDTEXTURE glBindTexture;

	PGLEND glEnd;
} GLFUNCLIST;

#endif//OPENGL32_PRIVATE_H
