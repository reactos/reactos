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
 * matching "real" function pointer from the NtCurrentTeb()->glDispatch table
 * and jmps to the address, leaving the stack alone and letting the "real"
 * function return for us.
 *
 * On other machines we use C to forward the calls (slow...)
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
/*#include <GL/gl.h>*/
/* -- complains about conflicting types of GL functions we export */
/* either fix that or put the GLxxx typedefs in here */

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

#if defined(__GNUC__) && defined(_M_IX86) /* use GCC extended inline asm */

# define X(func, ret, typeargs, args)                                 \
	void WINAPI func typeargs                                          \
	{                                                                 \
		__asm__(                                                      \
			"movl	%%fs:0x18,	%%eax"		"\n\t"                    \
			"addl	%0,			%%eax"		"\n\t"                    \
			"jmpl	*(%%eax)"				"\n\t"                    \
			:                                                         \
			: "n"(0x714+(GLIDX_##func*sizeof(PVOID))) );              \
	}

#elif defined(_MSC_VER) && defined(_M_IX86) /* use MSVC intel inline asm */

# define X(func, ret, typeargs, args)                                 \
	ret WINAPI func typeargs                                          \
	{                                                                 \
		__asm {                                                       \
			mov		eax,		fs:[00000018]                         \
			jmp		*GLIDX_##func(eax)                                \
		}                                                             \
	}

#else /* use C code */
#error C
# define X(func, ret, typeargs, args)                                 \
ret WINAPI func typeargs                                              \
{                                                                     \
	PVOID fn = (PVOID)(NtCurrentTeb()->glDispatch[GLIDX_##func]);     \
	return (ret)((ret (*) typeargs)fn)args;                           \
}

#endif

GLFUNCS_MACRO
#undef X

/* EOF */
