/*
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS kernel
 * FILE:                 lib/opengl32/gl.c
 * PURPOSE:              OpenGL32 lib, glXXX functions
 * PROGRAMMER:           Anich Gregor (blight)
 * UPDATE HISTORY:
 *                       Feb 2, 2004: Created
 */

/* FIXME: everything in this file ;-) */

/* On a x86 we call the ICD functions in a special-way:
 *
 * For every glXXX function we export a glXXX entry-point which loads the
 * matching "real" function pointer from the NtCurrentTeb()->glDispatchTable
 * for gl functions in teblist.h and for others it gets the pointer from
 * NtCurrentTeb()->glTable and jmps to the address, leaving the stack alone and
 * letting the "real" function return for us.
 * Royce has implemented this in NASM =D
 *
 * On other machines we use C to forward the calls (slow...)
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ntos/types.h>
#include <napi/teb.h>

#include "opengl32.h"

/* GL data types - x86 typedefs */
typedef unsigned int GLenum;
typedef unsigned char GLboolean;
typedef unsigned int GLbitfield;
typedef signed char GLbyte;
typedef short GLshort;
typedef int GLint;
typedef int GLsizei;
typedef unsigned char GLubyte;
typedef unsigned short GLushort;
typedef unsigned int GLuint;
typedef unsigned short GLhalf;
typedef float GLfloat;
typedef float GLclampf;
typedef double GLdouble;
typedef double GLclampd;
typedef void GLvoid;

void STDCALL glEmptyFunc0() {}
void STDCALL glEmptyFunc4( long l1 ) {}
void STDCALL glEmptyFunc8( long l1, long l2 ) {}
void STDCALL glEmptyFunc12( long l1, long l2, long l3 ) {}
void STDCALL glEmptyFunc16( long l1, long l2, long l3, long l4 ) {}
void STDCALL glEmptyFunc20( long l1, long l2, long l3, long l4, long l5 ) {}
void STDCALL glEmptyFunc24( long l1, long l2, long l3, long l4, long l5,
                            long l6 ) {}
void STDCALL glEmptyFunc28( long l1, long l2, long l3, long l4, long l5,
                            long l6, long l7 ) {}
void STDCALL glEmptyFunc32( long l1, long l2, long l3, long l4, long l5,
                            long l6, long l7, long l8 ) {}
void STDCALL glEmptyFunc36( long l1, long l2, long l3, long l4, long l5,
                            long l6, long l7, long l8, long l9 ) {}
void STDCALL glEmptyFunc40( long l1, long l2, long l3, long l4, long l5,
                            long l6, long l7, long l8, long l9, long l10 ) {}
void STDCALL glEmptyFunc44( long l1, long l2, long l3, long l4, long l5,
                            long l6, long l7, long l8, long l9, long l10,
                            long l11 ) {}
void STDCALL glEmptyFunc48( long l1, long l2, long l3, long l4, long l5,
                            long l6, long l7, long l8, long l9, long l10,
                            long l11, long l12 ) {}
void STDCALL glEmptyFunc52( long l1, long l2, long l3, long l4, long l5,
                            long l6, long l7, long l8, long l9, long l10,
                            long l11, long l12, long l13 ) {}
void STDCALL glEmptyFunc56( long l1, long l2, long l3, long l4, long l5,
                            long l6, long l7, long l8, long l9, long l10,
                            long l11, long l12, long l13, long l14 ) {}


# define X(func, ret, typeargs, args, icdidx, tebidx, stack)          \
ret STDCALL func typeargs                                             \
{                                                                     \
    PROC *table;                                                      \
	PROC fn;                                                          \
	if (tebidx >= 0)                                                  \
	{                                                                 \
		table = (PROC *)NtCurrentTeb()->glDispatchTable;              \
		fn = table[tebidx];                                           \
	}                                                                 \
	else                                                              \
	{                                                                 \
		table = (PROC *)NtCurrentTeb()->glTable;                      \
		fn = table[icdidx];                                           \
	}                                                                 \
	return (ret)((ret(*)typeargs)fn)args;                             \
}

GLFUNCS_MACRO

#undef X

/* EOF */
