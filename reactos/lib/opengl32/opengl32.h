/* $Id: opengl32.h,v 1.3 2004/02/01 17:18:48 royce Exp $
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

#define GLFUNCS_MACRO \
	X(glAccum) \
	X(glAddSwapHintRectWIN) \
	X(glArrayElement) \
	X(glBegin) \
	X(glBindTexture)

enum glfunc_indices
{
	GLIDX_INVALID = -1,
#define X(X) GLIDX_##X,
	GLFUNCS_MACRO
#undef X
	GLIDX_COUNT
};

extern const char* OPENGL32_funcnames[GLIDX_COUNT];

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

typedef struct tagGLPROCESSDATA
{
	int funclist_count;
	GLFUNCLIST* lists; // array of GLFUNCLIST pointers
} GLPROCESSDATA;

typedef struct tagGLTHREADDATA
{
	HDC hdc; // current HDC
	GLFUNCLIST* list; // *current* func list
	/* FIXME - what else do we need here? */
}; GLTHREADDATA;

extern DWORD OPENGL32_tls;
extern GLPROCESSDATA OPENGL32_processdata;

#endif//OPENGL32_PRIVATE_H
